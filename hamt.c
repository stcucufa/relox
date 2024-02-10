#include "hamt.h"

static HAMTNode* hamt_new_map(void) {
    HAMTNode* map = calloc(sizeof(HAMTNode), 32);
    for (size_t i = 0; i < 32; ++i) {
        map[i].key = VALUE_NONE;
    }
    return map;
}

void hamt_init(HAMT* hamt) {
    hamt->count = 0;
    hamt->root = hamt_new_map();
}

Value hamt_get(HAMT* hamt, Value key) {
    uint32_t hash = value_hash(key);
    HAMTNode* map = hamt->root;
    for (size_t i = 0; i < 6; ++i) {
        size_t index = hash & 0x1f;
        HAMTNode* node = &map[index];
        if (VALUE_IS_NONE(node->key)) {
            return VALUE_NONE;
        }
        if (!VALUE_IS_HAMT_NODE(node->key)) {
            return VALUE_EQUAL(node->key, key) ? node->content.value : VALUE_NONE;
        }
        map = node->content.map;
        hash >>= 5;
    }
    // TODO rehash
    return VALUE_NONE;
}

void hamt_set(HAMT* hamt, Value key, Value value) {
    uint32_t hash = value_hash(key);
    HAMTNode* map = hamt->root;
    for (size_t i = 0; i < 6; ++i) {
        size_t index = hash & 0x1f;
        HAMTNode* node = &map[index];
        if (VALUE_IS_NONE(node->key)) {
            node->key = key;
            node->content.value = value;
            hamt->count += 1;
            return;
        }
        if (!VALUE_IS_HAMT_NODE(node->key)) {
            if (VALUE_EQUAL(node->key, key)) {
                node->content.value = value;
            } else {
                // TODO j == index
                size_t j = (value_hash(node->key) >> (5 * i)) & 0x1f;
                HAMTNode* map = hamt_new_map();
                map[index].key = key;
                map[index].content.value = value;
                map[j].key = node->key;
                map[j].content.value = node->content.value;
                node->key = VALUE_HAMT_NODE;
                node->content.map = map;
                hamt->count += 1;
            }
            return;
        }
        map = node->content.map;
        hash >>= 5;
    }
    // TODO rehash
}

void hamt_free(HAMT* hamt) {
    // TODO
}
