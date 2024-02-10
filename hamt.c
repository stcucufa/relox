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
            // Add a new key/value pair to the node.

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
            for (size_t ii = j + 1; ii <= k; ++ii) {
                nodes[ii] = node->content.nodes[ii - 1];
            }
            free(node->content.nodes);
            node->content.nodes = nodes;
            hamt->count += 1;
            return;

        }
        node = &node->content.nodes[j];
        if (!VALUE_IS_HAMT_NODE(node->key)) {
            if (VALUE_EQUAL(node->key, key)) {
                // Update an existing key/value pair.

#ifdef DEBUG
                fputs("### hamt_set(", stderr);
                value_print(stderr, key);
                fputs(", ", stderr);
                value_print(stderr, value);
                fprintf(stderr, "), hash: 0x%08x (%02d), level %zu: update existing value ",
                    value_hash(key), hash & 0x1f, i);
                value_print(stderr, node->content.value);
                fputs("\n", stderr);
#endif
                node->content.value = value;

            } else {
                // Collision, need to convert key/value pair to a node.

#ifdef DEBUG
                fputs("### hamt_set(", stderr);
                value_print(stderr, key);
                fputs(", ", stderr);
                value_print(stderr, value);
                fprintf(stderr, "), hash: 0x%08x (%02d), level %zu: collision with key ",
                    value_hash(key), hash & 0x1f, i);
                value_print(stderr, node->key);
                fputs("\n", stderr);
#endif

                if (i == 5) {
                    // TODO rehash
                    exit(EXIT_FAILURE);
                }

                size_t new_mask = 1 << ((hash >> 5) & 0x1f);
                size_t previous_mask = 1 << (value_hash(node->key) >> (5 * (i + 1)));
                if (new_mask == previous_mask) {
                    // TODO more nodes
                    exit(EXIT_FAILURE);
                }

                Value bitmap = VALUE_HAMT_NODE;
                bitmap.as_int |= new_mask;
                bitmap.as_int |= previous_mask;

                HAMTNode* nodes = calloc(sizeof(HAMTNode), 2);
                size_t new_i = new_mask < previous_mask ? 0 : 1;
                nodes[new_i].key = key;
                nodes[new_i].content.value = value;

                size_t previous_i = new_mask < previous_mask ? 1 : 0;
                nodes[previous_i].key = node->key;
                nodes[previous_i].content.value = node->content.value;

                node->key = bitmap;
                node->content.nodes = nodes;
                hamt->count += 1;
            }
            return;
        }
        hash >>= 5;
    }
    // TODO rehash
}

static void hamt_free_node(HAMTNode* node) {
    size_t k = __builtin_popcount(VALUE_TO_HAMT_NODE_BITMAP(node->key));
    for (size_t i = 0; i < k; ++i) {
        HAMTNode* child_node = &node->content.nodes[i];
        if (VALUE_IS_HAMT_NODE(child_node->key)) {
            hamt_free_node(child_node);
        }
    }
#ifdef DEBUG
    fprintf(stderr, "--- hamt_free_node(): freed nodes (%zu)\n", k);
#endif
    free(node->content.nodes);
}

void hamt_free(HAMT* hamt) {
    hamt_free_node(&hamt->root);
    hamt_init(hamt);
}

#ifdef DEBUG
void hamt_debug_node(HAMTNode* node) {
    size_t k = __builtin_popcount(VALUE_TO_HAMT_NODE_BITMAP(node->key));
    for (size_t i = 0; i < k; ++i) {
        HAMTNode* child_node = &node->content.nodes[i];
        if (VALUE_IS_HAMT_NODE(child_node->key)) {
            hamt_debug_node(child_node);
        } else {
            value_print(stderr, child_node->key);
            fputs(": ", stderr);
            value_print(stderr, child_node->content.value);
            fputs(", ", stderr);
        }
    }
}

void hamt_debug(HAMT* hamt) {
    fputs("### { ", stderr);
    hamt_debug_node(&hamt->root);
    fprintf(stderr, "}, count: %zu\n", hamt->count);
}
#endif
