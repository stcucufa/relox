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
        HAMTNode* child_node = &node->content.nodes[i];
        if (VALUE_IS_HAMT_NODE(child_node->key)) {
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
    for (size_t i = 0; i < 5e7; ++i) {
        hamt_set(&hamt, VALUE_FROM_NUMBER(i), VALUE_TRUE);
        if ((i % 500000) == 0) {
            printf("%8zu #0x%016" PRIx64 "\n", i, value_hash(VALUE_FROM_NUMBER(i)));
        }
    }
    printf("Count: %zu, height: %zu\n", hamt.count, node_height(&hamt.root));
    hamt_free(&hamt);
    return EXIT_SUCCESS;
}
