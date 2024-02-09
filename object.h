#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <stdlib.h>

typedef struct {
    size_t length;
    uint32_t hash;
    char chars[];
} String;

String* string_copy(const char*, size_t);
String* string_concatenate(String*, String*);
String* string_exponent(String*, double);
String* string_from_number(double);
void string_free(String*);

#endif
