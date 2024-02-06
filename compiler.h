#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <stdbool.h>
#include "vm.h"

bool compile_chunk(const char*, Chunk*);

#endif
