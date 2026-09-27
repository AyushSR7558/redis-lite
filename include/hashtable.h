typedef struct HNode{
    uint64_t hcode;
    struct HNode *next;
} HNode;

typedef struct HTab{
    HNode **tab;
    size_t mask;
    size_t size;
} HTab;
 
typedef struct HMap{
    HTab old;
    HTab new;
    size_t resizing_pos;
} HMap;
