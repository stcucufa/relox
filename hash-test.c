#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hash-table.h"
#include "value.h"

void test_numbers(void) {
    HashTable table;
    hash_table_init(&table);

    for (size_t i = 0; i < 1131152; ++i) {
        hash_table_set(&table, VALUE_FROM_NUMBER(i), VALUE_FROM_NUMBER(0));
    }
    // hash_table_free(&table);
}

int main(int argc, char* argv[argc + 1]) {
    test_numbers();
    return EXIT_SUCCESS;
}
