#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include "value.h"
#include "object.h"

typedef struct {
    String* key;
    Value value;
} Entry;

#define HASH_TABLE_MAX_LOAD 0.75

typedef struct {
    size_t count;
    size_t capacity;
    Entry* entries;
} HashTable;

void hash_table_init(HashTable*);
bool hash_table_set(HashTable*, String*, Value);
bool hash_table_get(HashTable*, String*, Value*);
String* hash_table_find_string(HashTable*, String*);
bool hash_table_delete(HashTable*, String*);
void hash_table_free(HashTable*);

#endif
