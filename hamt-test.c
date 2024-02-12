#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hamt.h"
#include "value.h"

size_t node_height(HAMTNode* node) {
    size_t h = 0;
    size_t k = __builtin_popcount(VALUE_TO_HAMT_NODE_BITMAP(node->key));
    for (size_t i = 0; i < k; ++i) {
        HAMTNode* child_node = &node->content.children[i];
        if (VALUE_IS_HAMT_NODE_BITMAP(child_node->key)) {
            size_t h_child = 1 + node_height(child_node);
            if (h_child > h) {
                h = h_child;
            }
        }
    }
    return h;
}

int main(int argc, char* argv[argc + 1]) {
    HAMT hamt;
    hamt_init(&hamt);
    for (size_t i = 0; i < 1e7; ++i) {
        hamt_set(&hamt, VALUE_FROM_NUMBER(i), VALUE_TRUE);
        if ((i % 500000) == 0) {
            uint64_t hash = value_hash(VALUE_FROM_NUMBER(i));
            printf("%8zu #0x%016" PRIx64 " =", i, hash);
            for (size_t j = 0; j < HAMT_MAX_HEIGHT; ++j) {
                printf(" %02" PRId64, (hash >> (HAMT_HASH_BITS * j)) & HAMT_HASH_MASK);
            }
            puts("");
        }
    }
    printf("Count: %zu, height: %zu\n", hamt.count, node_height(&hamt.root));
    hamt_free(&hamt);
    return EXIT_SUCCESS;
}
