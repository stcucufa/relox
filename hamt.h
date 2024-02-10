#ifndef __HAMT_H__
#define __HAMT_H__

#include "value.h"

typedef struct HAMTNode {
    Value key;
    union {
        Value value;
        struct HAMTNode* nodes;
    } content;
} HAMTNode;

typedef struct {
    size_t count;
    HAMTNode root;
} HAMT;

void hamt_init(HAMT*);
Value hamt_get(HAMT*, Value);
Value hamt_get_string(HAMT*, String*);
void hamt_set(HAMT*, Value, Value);
void hamt_free(HAMT*);

#ifdef DEBUG
void hamt_debug(HAMT*);
#endif

#endif
