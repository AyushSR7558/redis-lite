#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT "1234"
#define k_max_msg 4096

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,  // mark the connection for deletion
};

struct Conn {
    int fd;
    uint32_t state;         // STATE_REQ, STATE_RES or STATE_END
    size_t rbuf_size;
    uint8_t rbuf[4 + k_max_msg];
    size_t wbuf_size;
    size_t wbuf_sent;
    uint8_t wbuf[4 + k_max_msg];
};

struct ConnMap {
    struct Conn **items;
    size_t size;
};

int get_listening_socket(void) {
    int listener = -1;
    int yes = 1;
    int rv;
    struct addrinfo hint, *res, *p;

    memset(&hint, 0, sizeof hint);
    hint.ai_flags = AI_PASSIVE;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = 0;

    if ((rv = getaddrinfo(NULL, PORT, &hint, &res)) != 0) {
        fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            close(listener);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (p == NULL) {  // never managed to bind
        return -1;
    }

    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}

void fd_set_nb(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(1);
    }
}

static void conn_put(struct ConnMap *map, struct Conn *conn) {
    size_t need = (size_t)conn->fd + 1;
    if (map->size < need) {
        struct Conn **tmp = realloc(map->items, need * sizeof(struct Conn *));
        if (!tmp) {
            perror("realloc");
            exit(1);
        }
        // zero the newly added slots (std::vector did this automatically)
        memset(tmp + map->size, 0, (need - map->size) * sizeof(struct Conn *));
        map->items = tmp;
        map->size = need;
    }
    map->items[conn->fd] = conn;
}

static int32_t accept_new_conn(struct ConnMap *map, int fd) {
    struct sockaddr_in client_addr;
    socklen_t socklen = sizeof(client_addr);
    memset(&client_addr, 0, sizeof client_addr);

    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        return -1;
    }

    fd_set_nb(connfd);

    struct Conn *conn = malloc(sizeof(struct Conn));
    if (!conn) {
        close(connfd);
        return -1;
    }

    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(map, conn);
    return 0;
}

static bool try_flush_buffer(struct Conn *conn) {
    ssize_t rv = 0;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);

    if (rv < 0 && errno == EAGAIN) {
        return false;  // got EAGAIN, stop
    }

    if (rv < 0) {
        printf("write() error\n");
        conn->state = STATE_END;
        return false;
    }

    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);

    if (conn->wbuf_sent == conn->wbuf_size) {
        // response fully sent, go back to reading requests
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }

    // still data left in wbuf, try to write again
    return true;
}

static void state_res(struct Conn *conn) {
    while (try_flush_buffer(conn)) {}
}

static bool try_one_request(struct Conn *conn) {
    if (conn->rbuf_size < 4) {
        return false;  // not enough data, retry next iteration
    }

    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg) {
        printf("too long\n");
        conn->state = STATE_END;  // can never be satisfied, drop the connection
        return false;
    }

    if (4 + (size_t)len > conn->rbuf_size) {
        return false;  // not enough data, retry next iteration
    }

    printf("client says: %.*s\n", (int)len, &conn->rbuf[4]);

    // echo response
    memcpy(&conn->wbuf[0], &len, 4);
    memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    conn->wbuf_size = 4 + len;

    // remove the request from the buffer
    size_t remain = conn->rbuf_size - 4 - len;
    if (remain) {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    conn->state = STATE_RES;
    state_res(conn);

    // continue the outer loop if the response was fully sent
    return (conn->state == STATE_REQ);
}

static bool try_fill_buffer(struct Conn *conn) {
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);

    if (rv < 0 && errno == EAGAIN) {
        return false;
    }

    if (rv < 0) {
        conn->state = STATE_END;
        return false;
    }

    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            printf("unexpected EOF\n");
        } else {
            printf("EOF\n");
        }
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    while (try_one_request(conn)) {}
    return (conn->state == STATE_REQ);
}

static void state_req(struct Conn *conn) {
    while (try_fill_buffer(conn)) {}
}

static void connection_io(struct Conn *conn) {
    if (conn->state == STATE_REQ) {
        state_req(conn);
    } else if (conn->state == STATE_RES) {
        state_res(conn);
    } else {
        assert(0);  // not expected
    }
}

int main(void) {
    int fd = get_listening_socket();
    if (fd < 0) {
        fprintf(stderr, "failed to create listening socket\n");
        return 1;
    }

    // map of all client connections, keyed by fd
    struct ConnMap fd2conn = { NULL, 0 };

    fd_set_nb(fd);

    // poll argument array (replaces std::vector<struct pollfd>)
    struct pollfd *poll_args = NULL;
    size_t poll_cap = 0;

    while (true) {
        // make sure poll_args can hold the listener + every connection
        size_t need = fd2conn.size + 1;
        if (poll_cap < need) {
            struct pollfd *tmp = realloc(poll_args, need * sizeof(struct pollfd));
            if (!tmp) {
                perror("realloc");
                return 1;
            }
            poll_args = tmp;
            poll_cap = need;
        }

        size_t nfds = 0;
        poll_args[nfds].fd = fd;
        poll_args[nfds].events = POLLIN;
        poll_args[nfds].revents = 0;
        nfds++;

        for (size_t i = 0; i < fd2conn.size; i++) {
            struct Conn *conn = fd2conn.items[i];
            if (!conn) {
                continue;
            }
            poll_args[nfds].fd = conn->fd;
            poll_args[nfds].events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            poll_args[nfds].events |= POLLERR;
            poll_args[nfds].revents = 0;
            nfds++;
        }

        int rv = poll(poll_args, (nfds_t)nfds, 1000);
        if (rv < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            return 1;
        }

        // process active connections
        for (size_t i = 1; i < nfds; i++) {
            if (poll_args[i].revents) {
                struct Conn *conn = fd2conn.items[poll_args[i].fd];
                connection_io(conn);
                if (conn->state == STATE_END) {
                    // client closed normally, or something went wrong
                    fd2conn.items[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }

        // accept a new connection if the listening fd is active
        if (poll_args[0].revents) {
            (void)accept_new_conn(&fd2conn, fd);
        }
    }

    return 0;
}
