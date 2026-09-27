#include<stdbool.h>
#include<assert.h>
#include<stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/hashtable.h"


const size_t resizing_work = 128;
const size_t max_load_factor = 8;

// Initialize the hash table
static void h_init(HTab * htab, size_t n) {
    assert(n > 0 && (n & (n - 1)) == 0);
    htab -> tab = (HNode **) calloc(sizeof(HNode), n);
    htab -> mask = n - 1;
    htab -> size = 0;
}

// Insert the node into hashtable
static void h_insert(HTab *htab, HNode *node) {
    size_t pos = htab -> mask & node -> hcode; 
    HNode* next = htab -> tab[pos];
    node -> next = next;
    htab -> tab[pos] = node;
}

// Return the address of the parent pointer 
// Can be used to delete the node
static HNode** h_lookup(HTab *htab, HNode *hnode, bool (*cmp) (HNode *, HNode *)) {
    // Get bucket first
    size_t pos = htab -> mask & hnode -> hcode;
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
static HNode* h_detach(HTab *htab, HNode **from) {
    if(!*from)  {
        return NULL;
    }
    HNode* node = *from;
    *from = (*from) -> next; 
    htab -> size--;
    return node; 
}

// function for gradually moving the node in one operation it moves resizing_work number of node
static void hm_help_resizing(HMap* hmap) {
    if(hmap -> old.tab == NULL) return ;
    size_t nwork = 0;

    while (nwork < resizing_work && hmap -> old.size > 0) {
        HNode** from = &hmap -> new.tab[hmap -> resizing_pos]; 

        if(!*from) {
            hmap -> resizing_pos++;
            continue;
        }

        h_insert(&hmap -> new, h_detach(&hmap -> old, from));
        nwork++;
    }

    if(hmap -> old.size == 0) {
        free(hmap -> old.tab);
        hmap -> old = (HTab){0};
    }
}

static void hm_start_resizing(HMap* hmap) {
    assert(hmap -> old.tab == 0);
    hmap -> old = hmap -> new;
    h_init(&hmap -> new, (hmap -> new.mask + 1) * 2);
    hmap -> resizing_pos = 0; 
}

void hm_insert(HMap *hmap, HNode *key) {
    if(!hmap -> new.tab) {
        h_init(&hmap -> new, 4);
    } 
   h_insert(&hmap -> new, key);
   size_t curr_load_factor = hmap -> new.size / (hmap -> new.mask + 1);

   if(curr_load_factor >= max_load_factor && (!hmap -> old.tab)) {
        hm_start_resizing(hmap);
   }
   hm_help_resizing(hmap);
}


// Look for hnode into both the hashtable
static HNode *hm_lookup(HMap *hmap, HNode *key, bool (*cmp) (HNode *, HNode *)) {
    hm_help_resizing(hmap);
    HNode **from = h_lookup(&hmap -> new, key, cmp);
    if(!from) {
        from = h_lookup(&hmap -> old, key, cmp);
    }
    return (from == NULL? NULL: *from);
}



/**
 * HTab -> HTab** tab, size_t size, size_t mask
 *
 * HNode -> HNode* next, int hcode
 *
 * HMap -> HTab* old, HTab* new, size_t resizing_pos = 0
 */

