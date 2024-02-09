#include <stdlib.h>

#include "array.h"
#include "hash-table.h"

void hash_table_init(HashTable* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = 0;
}

static Entry* hash_table_find_entry(Entry* entries, size_t capacity, String* key) {
    Entry* tombstone = 0;
    size_t index = (size_t)key->hash % capacity;
    for (;;) {
        Entry* entry = &entries[index];
        if (!entry->key) {
            if (VALUE_IS_NIL(entry->value)) {
                if (tombstone) {
                    return tombstone;
                }
                return entry;
            } else {
                if (!tombstone) {
                    tombstone = entry;
                }
            }
        } else if (entry->key == key) {
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

static void hash_table_adjust_capacity(HashTable* table, size_t capacity) {
    Entry* entries = calloc(capacity, sizeof(Entry));
    for (size_t i = 0; i < capacity; ++i) {
        entries[i].key = 0;
        entries[i].value = VALUE_NIL;
    }

#ifdef DEBUG
    fprintf(stderr, "+++ hash_table_adjust_capacity(): new capacity %zu for %p\n",
        capacity, (void*)table);
#endif

    table->count = 0;
    for (int i = 0; i < table->capacity; ++i) {
        Entry* entry = &table->entries[i];
        if (entry->key != 0) {
            Entry* dest = hash_table_find_entry(entries, capacity, entry->key);
            dest->key = entry->key;
            dest->value = entry->value;
            table->count += 1;
        }
    }

    free(table->entries);
    table->entries = entries;
    table->capacity = capacity;
}

bool hash_table_set(HashTable* table, String* key, Value value) {
    if (table->count + 1 > table->capacity * HASH_TABLE_MAX_LOAD) {
        size_t capacity = table->capacity < ARRAY_MIN_CAPACITY ?
            ARRAY_MIN_CAPACITY : ARRAY_GROW_FACTOR * table->capacity;
        hash_table_adjust_capacity(table, capacity);
    }

    Entry* entry = hash_table_find_entry(table->entries, table->capacity, key);
    bool new_key = entry->key == 0;
    if (new_key && VALUE_IS_NIL(entry->value)) {
        table->count += 1;
    }

    entry->key = key;
    entry->value = value;
    return new_key;
}

bool hash_table_get(HashTable* table, String* key, Value* value) {
    if (table->count == 0) {
        return false;
    }

    Entry* entry = hash_table_find_entry(table->entries, table->capacity, key);
    if (!entry->key) {
        return false;
    }
    *value = entry->value;
    return true;
}

String* hash_table_find_string(HashTable* table, String* key) {
    if (table->count == 0) {
        return 0;
    }

    size_t index = (size_t)key->hash % table->capacity;
    for (;;) {
        Entry* entry = &table->entries[index];
        if (!entry->key) {
            if (VALUE_IS_NIL(entry->value)) {
                return 0;
            }
        } else if (string_equal(entry->key, key)) {
            return entry->key;
        }
        index = (index + 1) % table->capacity;
    }
}

bool hash_table_delete(HashTable* table, String* key) {
    if (table->count == 0) {
        return false;
    }

    Entry* entry = hash_table_find_entry(table->entries, table->capacity, key);
    if (!entry->key) {
        return false;
    }
    entry->key = 0;
    entry->value = VALUE_TRUE;
    return true;
}

void hash_table_free(HashTable* table) {
#ifdef DEBUG
    fprintf(stderr, "--- hash_table_free() free %p.\n", (void*)table);
#endif
    free(table->entries);
    hash_table_init(table);
}
