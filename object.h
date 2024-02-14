#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

uint32_t bytes_hash(char*, size_t);

typedef struct {
    size_t length;
    uint32_t hash;
    char chars[];
} String;

String* string_new(size_t);
String* string_copy(const char*, size_t);
String* string_concatenate(String*, String*);
String* string_exponent(String*, double);
String* string_from_number(double);
bool string_equal(String*, String*);

#endif
