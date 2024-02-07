#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <stdlib.h>

typedef struct Obj {
    struct Obj* next;
} Obj;

typedef struct {
    Obj* obj;
    size_t length;
    char* chars;
} String;

String* string_copy(const char*, size_t);
String* string_concatenate(String*, String*);

#endif
