struct HNode {
    uint64_t hcode = 0;
    struct HNode *next = NULL;
}

struct HTab{
    HNode **tab = NULL;
    size_t mask = 0;
    size_t size = 0;
}
