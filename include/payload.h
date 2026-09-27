#define RES_OK 0
#define RES_NX 1


const size_t max_msg;

typedef struct Entry {
    HNode node;
    char *key;
    char *value;
} Entry;
