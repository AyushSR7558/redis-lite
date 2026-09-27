#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "./../include/hashtable.h"
#include "./../include/payload.h"

#define offset(type, member) ((size_t)&(((type *)0)->member))

#define container_of(ptr, type, member) ({                    \
    const typeof(((type *)0)->member) *_mptr = (ptr);         \
    (type *)((char *)_mptr - offset(type, member));           \
})                                  

struct {
    HMap db;
} g_key;


static bool cmp(HNode *lhs, HNode *rhs) {
    Entry *le = container_of(lhs, Entry, node);
    Entry *re = container_of(rhs, Entry, node);

    return lhs -> hcode == rhs -> hcode && !strcmp(le -> key, re -> key);
}

static uint32_t do_get (char *cmd , char *res, size_t* reslen) {
     Entry key = {0};
     key.node.hcode = hash_key(cmd, strlen(cmd));
     key.key = cmd;
     HNode *node = hm_lookup(&g_key.db, &key.node, &cmp);
     if(node)  {
         return RES_NX;
     }

     const char* val = container_of(node, Entry, node) -> value;
     assert(strlen(val) + 1 <= *reslen);
     strcpy(res, val);
     *reslen = strlen(res) + 1;
     return RES_OK;
}

