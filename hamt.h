#ifndef __HAMT_H__
#define __HAMT_H__

#include "value.h"

#define MAX_HEIGHT 13
#define HASH_BITS 5
#define HASH_MASK 0x1f

// A node in the tree is either an entry (key/value pair) or a map with a
// bitmap (stored in the key, which is of type VALUE_HAMT_NODE) and a list
// of 1 to 32 child nodes. The bitmap indicates which slots have a node or
// not to avoid storing 32 pointers in each node.
typedef struct HAMTNode {
    Value key;
    union {
        Value value;
        struct HAMTNode* nodes;
    } content;
} HAMTNode;

// The HAMT keeps its count and a regular root node (it is not resized as in
// the original paper).
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
