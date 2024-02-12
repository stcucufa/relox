#include "hamt.h"

static HAMTNode* hamt_get_node(size_t k) {
    return calloc(sizeof(HAMTNode), k);
}

static void hamt_discard_node(size_t k, HAMTNode* node) {
    (void)k;
    free(node);
}

// Initialize the HAMT.
void hamt_init(HAMT* hamt) {
    hamt->count = 0;
    hamt->root.key = VALUE_HAMT_NODE_BITMAP;
    hamt->root.content.children = 0;
}

// Get the value for a key from the HAMT; return VALUE_NONE if it was not found.
// TODO handle hash collisions.
Value hamt_get(HAMT* hamt, Value key) {
    uint64_t hash = value_hash(key);
    HAMTNode node = hamt->root;
    for (size_t i = 0; i < HAMT_MAX_HEIGHT; ++i) {
        // Get 5 bits of hash and make a mask for the position in the bitmap.
        uint32_t mask = 1 << (hash & HAMT_HASH_MASK);
        uint32_t bitmap = VALUE_TO_HAMT_NODE_BITMAP(node.key);
        if ((bitmap & mask) == 0) {
            // The bit is 0 so the key is not present in the trie.
            return VALUE_NONE;
        }
        // Get the position in the map from the popcount of the bits up to
        // the position of the hash value. mask - 1 turns a mask like
        // 00100000 into 00011111.
        size_t j = __builtin_popcount(bitmap & (mask - 1));
        node = node.content.children[j];
        if (!VALUE_IS_HAMT_NODE_BITMAP(node.key)) {
            // This is an entry, so if the keys match then a value was found.
            return VALUE_EQUAL(node.key, key) ? node.content.value : VALUE_NONE;
        }
        // Keep going down with the next 5 bits of the hash.
        hash >>= HAMT_HASH_BITS;
    }
    return VALUE_NONE;
}

// Get the value for a string from the trie (comparing the actual strings and
// not just the value; this is used for string interning before a new string
// is added).
Value hamt_get_string(HAMT* hamt, String* string) {
    uint64_t hash = string->hash;
    HAMTNode node = hamt->root;
    for (size_t i = 0; i < HAMT_MAX_HEIGHT; ++i) {
        uint32_t mask = 1 << (hash & HAMT_HASH_MASK);
        uint32_t bitmap = VALUE_TO_HAMT_NODE_BITMAP(node.key);
        if ((bitmap & mask) == 0) {
            return VALUE_NONE;
        }
        size_t j = __builtin_popcount(bitmap & (mask - 1));
        node = node.content.children[j];
        if (!VALUE_IS_HAMT_NODE_BITMAP(node.key)) {
            return string_equal(VALUE_TO_STRING(node.content.value), string) ?
                node.content.value : VALUE_NONE;
        }
        hash >>= HAMT_HASH_BITS;
    }
    return VALUE_NONE;
}

// When inserting a new value, it may collide with a previously inserted entry.
// Replace that entry with a new map by getting the next 5 bits of both hashes,
// and keep going while there are collisions.
// TODO rehash if the full hashes collide.
static void hamt_resolve_collision(HAMTNode* node, Value key, Value value, Value previous_key,
    Value previous_value, uint64_t hash, size_t i) {
    if (i >= HAMT_MAX_HEIGHT) {
        exit(EXIT_FAILURE);
    }

    // Bit positions for new and previous values in the bitmap of the new node.
    size_t new_mask = 1 << ((hash >> HAMT_HASH_BITS) & HAMT_HASH_MASK);
    size_t previous_mask = 1 << (value_hash(previous_key) >> (HAMT_HASH_BITS * (i + 1)));
    // Update the bitmap in the node.
    node->key = VALUE_HAMT_NODE_BITMAP;
    node->key.as_int |= new_mask;
    node->key.as_int |= previous_mask;

    if (new_mask == previous_mask) {
        // Both entries have the same position, so insert yet another map in
        // between.
        node->content.children = hamt_get_node(1);
        hamt_resolve_collision(
            node->content.children, key, value, previous_key, previous_value, hash >> HAMT_HASH_BITS, i + 1
        );
    } else {
        // The entries have different positions, so add the two values to the
        // new map.
        size_t new_i = new_mask < previous_mask ? 0 : 1;
        size_t previous_i = new_mask < previous_mask ? 1 : 0;
        node->content.children = hamt_get_node(2);
        node->content.children[new_i].key = key;
        node->content.children[new_i].content.value = value;
        node->content.children[previous_i].key = previous_key;
        node->content.children[previous_i].content.value = previous_value;
    }
}

// Set a value for a key in the HAMT. If the key is already present, the value
// is updated, otherwise a new entry is added, possibly creating intermediary
// maps along the way.
// TODO rehash in case of collision.
void hamt_set(HAMT* hamt, Value key, Value value) {
    uint64_t hash = value_hash(key);
    HAMTNode* node = &hamt->root;
    for (size_t i = 0; i < 6; ++i) {
        uint64_t mask = 1 << (hash & HAMT_HASH_MASK);
        uint32_t bitmap = VALUE_TO_HAMT_NODE_BITMAP(node->key);
        size_t j = __builtin_popcount(bitmap & (mask - 1));
        if ((bitmap & mask) == 0) {
            // A free slot was found so add a new entry in the map.
            // Set the bit in the bitmap.
            node->key.as_int |= (uint64_t)mask;
            // Copy the k values to a new array to keep them in order.
            size_t k = __builtin_popcount(bitmap);
            HAMTNode* children = hamt_get_node(k + 1);
            for (size_t ii = 0; ii < j; ++ii) {
                children[ii] = node->content.children[ii];
            }
            // Insert the new entry at the right position.
            children[j] = (HAMTNode){ .key = key, .content = { .value = value } };
            for (size_t ii = j + 1; ii <= k; ++ii) {
                children[ii] = node->content.children[ii - 1];
            }
            hamt_discard_node(k, node->content.children);
            node->content.children = children;
            hamt->count += 1;
            return;

        }

        // The slot is occupied by an entry or a map.
        node = &node->content.children[j];
        if (!VALUE_IS_HAMT_NODE_BITMAP(node->key)) {
            // This is an entry which needs to be updated if the keys map;
            // otherwise, a new map needs to be inserted instead for both the
            // previous entry and the new entry (see above).
            if (VALUE_EQUAL(node->key, key)) {
                node->content.value = value;
            } else {
                hamt_resolve_collision(node, key, value, node->key, node->content.value, hash, i);
                hamt->count += 1;
            }
            return;
        }

        // Keep going down with the next 5 bits of the hash.
        hash >>= HAMT_HASH_BITS;
    }
}

// Free a node and its children.
static void hamt_free_node(HAMTNode* node) {
    size_t k = __builtin_popcount(VALUE_TO_HAMT_NODE_BITMAP(node->key));
    for (size_t i = 0; i < k; ++i) {
        HAMTNode* child_node = &node->content.children[i];
        if (VALUE_IS_HAMT_NODE_BITMAP(child_node->key)) {
            hamt_free_node(child_node);
        }
    }
    free(node->content.children);
}

// Free the HAMT and all its nodes.
void hamt_free(HAMT* hamt) {
    hamt_free_node(&hamt->root);
#ifdef DEBUG
    fprintf(stderr, "--- hamt_free_node() (count: %zu)\n", hamt->count);
#endif
    hamt_init(hamt);
}

#ifdef DEBUG
void hamt_debug_node(HAMTNode* node) {
    size_t k = __builtin_popcount(VALUE_TO_HAMT_NODE_BITMAP(node->key));
    for (size_t i = 0; i < k; ++i) {
        HAMTNode* child_node = &node->content.children[i];
        if (VALUE_IS_HAMT_NODE_BITMAP(child_node->key)) {
            hamt_debug_node(child_node);
        } else {
            value_printf(stderr, child_node->key, true);
            fputs(": ", stderr);
            value_printf(stderr, child_node->content.value, true);
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
