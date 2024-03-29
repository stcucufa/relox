#include <stdlib.h>

#include "hamt.h"

// Initialize the HAMT.
void hamt_init(HAMT* hamt) {
    hamt->count = 0;
    hamt->root.key = VALUE_HAMT_NODE;
    hamt->root.content.nodes = 0;
    hamt->root.refcount = 1;
}

// Get the value for a key from the HAMT; return VALUE_NONE if it was not found.
// TODO handle hash collisions.
Value hamt_get(HAMT* hamt, Value key) {
    uint32_t hash = value_hash(key);
    HAMTNode node = hamt->root;
    for (size_t i = 0; i < 6; ++i) {
        // Get 5 bits of hash and make a mask for the position in the bitmap.
        uint32_t mask = 1 << (hash & 0x1f);
        uint32_t bitmap = VALUE_TO_HAMT_NODE_BITMAP(node.key);
        if ((bitmap & mask) == 0) {
            // The bit is 0 so the key is not present in the trie.
            return VALUE_NONE;
        }
        // Get the position in the map from the popcount of the bits up to
        // the position of the hash value. mask - 1 turns a mask like
        // 00100000 into 00011111.
        size_t j = __builtin_popcount(bitmap & (mask - 1));
        node = node.content.nodes[j];
        if (!VALUE_IS_HAMT_NODE(node.key)) {
            // This is an entry, so if the keys match then a value was found.
            return VALUE_EQUAL(node.key, key) ? node.content.value : VALUE_NONE;
        }
        // Keep going down with the next 5 bits of the hash.
        hash >>= 5;
    }
    return VALUE_NONE;
}

// Get the value for a string from the trie (comparing the actual strings and
// not just the value; this is used for string interning before a new string
// is added).
Value hamt_get_string(HAMT* hamt, String* string) {
    uint32_t hash = string->hash;
    HAMTNode node = hamt->root;
    for (size_t i = 0; i < 6; ++i) {
        uint32_t mask = 1 << (hash & 0x1f);
        uint32_t bitmap = VALUE_TO_HAMT_NODE_BITMAP(node.key);
        if ((bitmap & mask) == 0) {
            return VALUE_NONE;
        }
        size_t j = __builtin_popcount(bitmap & (mask - 1));
        node = node.content.nodes[j];
        if (!VALUE_IS_HAMT_NODE(node.key)) {
            return string_equal(VALUE_TO_STRING(node.content.value), string) ?
                node.content.value : VALUE_NONE;
        }
        hash >>= 5;
    }
    return VALUE_NONE;
}

// Reverse lookup: find a key for a value.
static Value hamt_find_key_in_node(HAMTNode* node, Value value) {
    size_t k = __builtin_popcount(VALUE_TO_HAMT_NODE_BITMAP(node->key));
    for (size_t i = 0; i < k; ++i) {
        HAMTNode* child_node = &node->content.nodes[i];
        if (VALUE_IS_HAMT_NODE(child_node->key)) {
            return hamt_find_key_in_node(child_node, value);
        } else if (VALUE_EQUAL(value, child_node->content.value)) {
            return child_node->key;
        }
    }
    return VALUE_NONE;
}

Value hamt_find_key(HAMT* hamt, Value value) {
    return hamt_find_key_in_node(&hamt->root, value);
}

// When inserting a new value, it may collide with a previously inserted entry.
// Replace that entry with a new map by getting the next 5 bits of both hashes,
// and keep going while there are collisions.
// TODO rehash if the full hashes collide.
static void hamt_resolve_collision(HAMTNode* node, HAMTNode* newn, Value key, Value value,
    Value previous_key, Value previous_value, uint32_t hash, size_t i) {
    if (i >= 6) {
        exit(EXIT_FAILURE);
    }

    // Bit positions for new and previous values in the bitmap of the new node.
    size_t new_mask = 1 << ((hash >> 5) & 0x1f);
    size_t previous_mask = 1 << (value_hash(previous_key) >> (5 * (i + 1)));
    // Update the bitmap in the node.
    newn->key = VALUE_HAMT_NODE;
    newn->key.as_int |= new_mask;
    newn->key.as_int |= previous_mask;

    if (new_mask == previous_mask) {
        // Both entries have the same position, so insert yet another map in
        // between.
        node->content.nodes = malloc(sizeof(HAMTNode));
        if (newn != node) {
            newn->content.nodes = malloc(sizeof(HAMTNode));
        }
        hamt_resolve_collision(
            node->content.nodes, newn->content.nodes,
            key, value, previous_key, previous_value, hash >> 5, i + 1
        );
    } else {
        // The entries have different positions, so add the two values to the
        // new map.
        size_t new_i = new_mask < previous_mask ? 0 : 1;
        size_t previous_i = new_mask < previous_mask ? 1 : 0;
        newn->content.nodes = calloc(sizeof(HAMTNode), 2);
        newn->content.nodes[new_i].refcount = 1;
        newn->content.nodes[new_i].key = key;
        newn->content.nodes[new_i].content.value = value;
        newn->content.nodes[previous_i].refcount = 1;
        newn->content.nodes[previous_i].key = previous_key;
        newn->content.nodes[previous_i].content.value = previous_value;
    }
}

// Set a value for a key in the HAMT. If the key is already present, the value
// is updated, otherwise a new entry is added, possibly creating intermediary
// maps along the way.
// TODO rehash in case of collision.
void hamt_set(HAMT* hamt, Value key, Value value) {
    uint32_t hash = value_hash(key);
    HAMTNode* node = &hamt->root;
    for (size_t i = 0; i < 6; ++i) {
        uint32_t mask = 1 << (hash & 0x1f);
        uint32_t bitmap = VALUE_TO_HAMT_NODE_BITMAP(node->key);
        size_t j = __builtin_popcount(bitmap & (mask - 1));
        if ((bitmap & mask) == 0) {
            // A free slot was found so add a new entry in the map.
            // Set the bit in the bitmap.
            node->key.as_int |= (uint64_t)mask;
            // Copy the k values to a new array to keep them in order.
            size_t k = __builtin_popcount(bitmap);
            HAMTNode* nodes = calloc(sizeof(HAMTNode), k + 1);
            for (size_t ii = 0; ii < j; ++ii) {
                nodes[ii] = node->content.nodes[ii];
                nodes[ii].refcount += 1;
            }
            // Insert the new entry at the right position.
            nodes[j] = (HAMTNode){ .refcount = 1, .key = key, .content = { .value = value } };
            for (size_t ii = j + 1; ii <= k; ++ii) {
                nodes[ii] = node->content.nodes[ii - 1];
                nodes[ii].refcount += 1;
            }
            // TODO keep a pool of nodes instead of allocating/freeing.
            free(node->content.nodes);
            node->content.nodes = nodes;
            hamt->count += 1;
            return;
        }

        // The slot is occupied by an entry or a map.
        node = &node->content.nodes[j];
        if (!VALUE_IS_HAMT_NODE(node->key)) {
            // This is an entry which needs to be updated if the keys match;
            // otherwise, a new map needs to be inserted instead for both the
            // previous entry and the new entry (see above).
            if (VALUE_EQUAL(node->key, key)) {
                node->content.value = value;
            } else {
                hamt_resolve_collision(node, node, key, value, node->key, node->content.value, hash, i);
                hamt->count += 1;
            }
            return;
        }

        // Keep going down with the next 5 bits of the hash.
        hash >>= 5;
    }
}

// Persistent add: create a new HAMT with this key/value, sharing as many nodes
// with the original HAMT as possible.
HAMT* hamt_with(HAMT* hamt, Value key, Value value) {
    HAMT* newh = malloc(sizeof(HAMT));
    hamt_init(newh);
    newh->count = hamt->count;

    uint32_t hash = value_hash(key);
    HAMTNode* node = &hamt->root;
    HAMTNode* newn = &newh->root;
    for (size_t i = 0; i < 6; ++i) {
        uint32_t mask = 1 << (hash & 0x1f);
        uint32_t bitmap = VALUE_TO_HAMT_NODE_BITMAP(node->key);
        size_t j = __builtin_popcount(bitmap & (mask - 1));
        size_t k = __builtin_popcount(bitmap);
        newn->key = node->key;

        if ((bitmap & mask) == 0) {
            // A free slot was found so add a new entry to the new map.
            newn->key.as_int |= (uint64_t)mask;
            // Copy the k values to a new array to keep them in order.
            HAMTNode* nodes = calloc(sizeof(HAMTNode), k + 1);
            for (size_t ii = 0; ii < j; ++ii) {
                nodes[ii] = node->content.nodes[ii];
                nodes[ii].refcount += 1;
            }
            // Insert the new entry at the right position.
            nodes[j] = (HAMTNode){ .refcount = 1, .key = key, .content = { .value = value } };
            for (size_t ii = j + 1; ii <= k; ++ii) {
                nodes[ii] = node->content.nodes[ii - 1];
                nodes[ii].refcount += 1;
            }
            newn->content.nodes = nodes;
            newh->count += 1;
            return newh;
        }

        // Copy the original node.
        newn->content.nodes = calloc(sizeof(HAMTNode), k);
        for (size_t ii = 0; ii < k; ++ii) {
            newn->content.nodes[ii] = node->content.nodes[ii];
            newn->content.nodes[ii].refcount += 1;
        }

        // The slot is occupied by an entry or a map.
        node = &node->content.nodes[j];
        newn = &newn->content.nodes[j];

        if (!VALUE_IS_HAMT_NODE(node->key)) {
            // This is an entry which needs to be updated if the keys match;
            // otherwise, a new map needs to be inserted instead for both the
            // previous entry and the new entry (see above).
            if (VALUE_EQUAL(node->key, key)) {
                newn->content.value = value;
            } else {
                hamt_resolve_collision(node, newn, key, value, node->key, node->content.value, hash, i);
                hamt->count += 1;
            }
            return newh;
        }

        // Keep going down with the next 5 bits of the hash.
        hash >>= 5;
    }

    return newh;
}

// Free a node and its children.
static void hamt_free_node(HAMTNode* node) {
    node->refcount -= 1;
    if (node->refcount > 0) {
        return;
    }

    size_t k = __builtin_popcount(VALUE_TO_HAMT_NODE_BITMAP(node->key));
    for (size_t i = 0; i < k; ++i) {
        HAMTNode* child_node = &node->content.nodes[i];
        if (VALUE_IS_HAMT_NODE(child_node->key)) {
            hamt_free_node(child_node);
        }
    }
    free(node->content.nodes);
}

// Free the HAMT and all its nodes.
void hamt_free(HAMT* hamt) {
    hamt_free_node(&hamt->root);
#ifdef DEBUG
    fprintf(stderr, "--- hamt_free_node(): freed nodes (%zu)\n", hamt->count);
#endif
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
            value_print_debug(stderr, child_node->key, true);
            fputs(": ", stderr);
            value_print_debug(stderr, child_node->content.value, true);
            fputs(", ", stderr);
        }
    }
}

// Dump the whole hash and its count.
void hamt_debug(HAMT* hamt) {
    fputs("### { ", stderr);
    hamt_debug_node(&hamt->root);
    fprintf(stderr, "}, count: %zu\n", hamt->count);
}
#endif
