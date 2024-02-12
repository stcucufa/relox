#include <stdlib.h>

#include "array.h"

void byte_array_init(ByteArray* array) {
    array->count = 0;
    array->capacity = 0;
    array->items = 0;
}

void byte_array_push(ByteArray* array, uint8_t byte) {
    if (array->count == array->capacity) {
        array->capacity = array->capacity < ARRAY_MIN_CAPACITY ?
            ARRAY_MIN_CAPACITY : ARRAY_GROW_FACTOR * array->capacity;
        array->items = realloc(array->items, array->capacity);
#ifdef DEBUG
        fprintf(stderr, "+++ byte_array_push() growing %p to %zu bytes.\n",
            (void*) array->items, array->capacity);
#endif
    }
    array->items[array->count] = byte;
    array->count += 1;
}

void byte_array_free(ByteArray* array) {
#ifdef DEBUG
    fprintf(stderr, "--- byte_array_free() free %p (%zu/%zu items).\n",
        (void*) array->items, array->count, array->capacity);
#endif
    free(array->items);
    byte_array_init(array);
}

void number_array_init(NumberArray* array) {
    array->count = 0;
    array->capacity = 0;
    array->items = 0;
}

void number_array_push(NumberArray* array, size_t n) {
    size_t size = array->count * sizeof(size_t);
    if (size == array->capacity) {
        size_t min_size = ARRAY_MIN_CAPACITY * sizeof(size_t);
        array->capacity = size < min_size ? min_size : ARRAY_GROW_FACTOR * size;
        array->items = realloc(array->items, array->capacity);
#ifdef DEBUG
        fprintf(stderr, "+++ number_array_push() growing %p to %zu bytes.\n",
            (void*) array->items, array->capacity);
#endif
    }
    array->items[array->count] = n;
    array->count += 1;
}

void number_array_free(NumberArray* array) {
#ifdef DEBUG
    fprintf(stderr, "--- number_array_free() free %p (count: %zu, size: %zu).\n",
        (void*) array->items, array->count, array->capacity);
#endif
    free(array->items);
    number_array_init(array);
}

void value_array_init(ValueArray* array) {
    array->count = 0;
    array->capacity = 0;
    array->items = 0;
}

void value_array_push(ValueArray* array, Value v) {
    size_t size = array->count * sizeof(Value);
    if (size == array->capacity) {
        size_t min_size = ARRAY_MIN_CAPACITY * sizeof(Value);
        array->capacity = size < min_size ? min_size : ARRAY_GROW_FACTOR * size;
        array->items = realloc(array->items, array->capacity);
#ifdef DEBUG
        fprintf(stderr, "+++ value_array_push() growing %p to %zu bytes.\n",
            (void*) array->items, array->capacity);
#endif
    }
    array->items[array->count] = v;
    array->count += 1;
}

Value value_array_pop(ValueArray* array) {
    if (array->count == 0) {
        return VALUE_NONE;
    }
    array->count -= 1;
    return array->items[array->count];
}

void value_array_free(ValueArray* array) {
#ifdef DEBUG
    fprintf(stderr, "--- value_array_free() free %p (count: %zu, size: %zu).\n",
        (void*) array->items, array->count, array->capacity);
#endif
    free(array->items);
    value_array_init(array);
}
