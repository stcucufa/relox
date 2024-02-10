#include "hamt.h"

void hamt_init(HAMT* hamt) {
    hamt->count = 0;
    hamt->root.key = VALUE_HAMT_NODE;
    hamt->root.content.nodes = 0;
}

Value hamt_get(HAMT* hamt, Value key) {
    uint32_t hash = value_hash(key);
    HAMTNode node = hamt->root;
    for (size_t i = 0; i < 6; ++i) {
        uint32_t mask = 1 << (hash & 0x1f);
        uint32_t bitmap = VALUE_TO_HAMT_NODE_BITMAP(node.key);
        if ((bitmap & mask) == 0) {
#ifdef DEBUG
            fputs("### hamt_get(", stderr);
            value_print(stderr, key);
            fprintf(stderr, "), hash: 0x%08x (%02d), level %zu: hit 0 bit\n",
                value_hash(key), hash & 0x1f, i);
            fprintf(stderr, "### bitmask: %08x\n", VALUE_TO_HAMT_NODE_BITMAP(node.key));
#endif
            return VALUE_NONE;
        }
        size_t j = __builtin_popcount(bitmap & (mask - 1));
        node = node.content.nodes[j];
        if (!VALUE_IS_HAMT_NODE(node.key)) {
#ifdef DEBUG
            fputs("### hamt_get(", stderr);
            value_print(stderr, key);
            fprintf(stderr, "), hash: 0x%08x (%02d), level %zu: found ",
                value_hash(key), hash & 0x1f, i);
            value_print(stderr, node.content.value);
            fputs("\n", stderr);
#endif
            return VALUE_EQUAL(node.key, key) ? node.content.value : VALUE_NONE;
        }
        hash >>= 5;
    }
    // TODO rehash
    return VALUE_NONE;
}

void hamt_set(HAMT* hamt, Value key, Value value) {
    uint32_t hash = value_hash(key);
    HAMTNode* node = &hamt->root;
    for (size_t i = 0; i < 6; ++i) {
        uint32_t mask = 1 << (hash & 0x1f);
        uint32_t bitmap = VALUE_TO_HAMT_NODE_BITMAP(node->key);
        size_t j = __builtin_popcount(bitmap & (mask - 1));
        if ((bitmap & mask) == 0) {
#ifdef DEBUG
            fputs("### hamt_set(", stderr);
            value_print(stderr, key);
            fputs(", ", stderr);
            value_print(stderr, value);
            fprintf(stderr, "), hash: 0x%08x (%02d), level %zu: hit 0 bit\n",
                value_hash(key), hash & 0x1f, i);
#endif
            node->key.as_int |= (uint64_t)mask;
#ifdef DEBUG
            fprintf(stderr, "### bitmask: %08x\n", VALUE_TO_HAMT_NODE_BITMAP(node->key));
#endif
            size_t k = __builtin_popcount(bitmap);
            HAMTNode* nodes = calloc(sizeof(HAMTNode), k + 1);
            for (size_t ii = 0; ii < j; ++ii) {
                nodes[ii] = node->content.nodes[ii];
            }
            nodes[j] = (HAMTNode){ .key = key, .content = { .value = value } };
            for (size_t ii = j + 1; ii < k; ++ii) {
                nodes[ii + 1] = node->content.nodes[ii];
            }
            free(node->content.nodes);
            node->content.nodes = nodes;
            hamt->count += 1;
            return;
        }
        node = &node->content.nodes[j];
        if (!VALUE_IS_HAMT_NODE(node->key)) {
            if (VALUE_EQUAL(node->key, key)) {
                node->content.value = value;
            } else {
                // TODO collision
                hamt->count += 1;
            }
            return;
        }
        hash >>= 5;
    }
    // TODO rehash
}

void hamt_free(HAMT* hamt) {
    // TODO
}
