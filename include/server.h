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

/* Replaces std::vector<Conn *>: a growable array indexed by fd. */
struct ConnMap {
    struct Conn **items;
    size_t size;
};

