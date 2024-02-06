#include <stdlib.h>

#include "vm.h"

int main(int argc, char* argv[argc + 1]) {
    Chunk chunk;
    chunk_init(&chunk);
    chunk_add_byte(&chunk, op_zero, 1);
    chunk_add_byte(&chunk, op_constant, 1);
    chunk_add_byte(&chunk, (uint8_t)chunk_add_constant(&chunk, 3.14159265358979323846), 1);
    chunk_add_byte(&chunk, op_constant, 3);
    chunk_add_byte(&chunk, (uint8_t)chunk_add_constant(&chunk, 2.718281828459045), 3);
#ifdef DEBUG
    chunk_debug(&chunk, "test");
#endif
    chunk_free(&chunk);
    return EXIT_SUCCESS;
}
