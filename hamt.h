#ifndef __HAMT_H__
#define __HAMT_H__

#include "value.h"

typedef struct HAMTNode {
    Value key;
    union {
        Value value;
        struct HAMTNode* map;
    } content;
} HAMTNode;

typedef struct {
    size_t count;
    HAMTNode* root;
} HAMT;

void hamt_init(HAMT*);
Value hamt_get(HAMT*, Value);
void hamt_set(HAMT*, Value, Value);
void hamt_free(HAMT*);

#endif
