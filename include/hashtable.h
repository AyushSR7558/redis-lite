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

void hm_insert(HMap *hmap, HNode *key);

HNode *hm_pop(HMap *hmap, HNode *key, bool (*cmp) (HNode *, HNode *));

HNode *hm_lookup(HMap *hmap, HNode *key, bool (*cmp) (HNode *, HNode *));

uint64_t hash_key (const char* key, size_t len);


