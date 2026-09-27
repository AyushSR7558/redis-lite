#include "../include/hashtable.h"

// Initialize the hash table
static void h_init(HTab * htab, size_t n) {
    assert(n > 0 && (n & (n - 1)) == 0);
    htab -> tab = (HNode **) calloc(sizeof(HNode), n);
    htab -> mask = n - 1;
    htab -> size = 0;
}

// Insert the node into hashtable
static void h_insert(HTab *htab, HNode *node) {
    size_t pos = htab -> mask & HNode -> hcode; 
    HNode* next = htab -> tab[pos];
    node -> next = next;
    htab -> tab[pos] = node;
}

// Return the address of the parent pointer 
// Can be used to delete the node
static HNode** lookup(HTab *htab, HNode *hnode, bool (*cmp) (HNode *, HNode *)) {
    // Get bucket first
    size_pos = htab -> mask & hnode -> hcode;
    HNode** from = &(htab -> tab[pos]);

    while(*from) {
        if(cmp(*from,  hnode)) {
            return from;
        }

        from = &(*from) -> next;
    }

    return NULL;
}


// Detach a node from the bucket
static void h_detach(HTab *htab, HNode **from) {
    if(!*from)  {
        return NULL;
    }
    HNode node = *from;
    *from = (*from) -> next; 
    htab -> size--;
    return node; 
}




/**
 * HTab -> HTab** tab, size_t size, size_t mask
 *
 * HNode -> HNode* next, int hcode
 */

