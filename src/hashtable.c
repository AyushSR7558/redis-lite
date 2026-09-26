#include "../include/hashtable.h"


static void h_init(HTab * htab, size_t n) {
    assert(n > 0 && (n & (n - 1)) == 0);
    htab -> tab = (HNode **) calloc(sizeof(HNode), n);
    htab -> mask = n - 1;
    htab -> size = 0;
}

static void h_insert(HTab *htab, HNode *node) {
    size_t pos = htab -> mask & HNode -> hcode;
    node -> next = htab -> tab[pos];
    htabl -> tab[pos] = node;
    htab -> size++;
}

}
