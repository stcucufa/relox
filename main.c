#include <stdlib.h>

#include "vm.h"

int main(int argc, char* argv[argc + 1]) {
    VM vm;
    Chunk chunk;

    chunk_init(&chunk);
    chunk_add_byte(&chunk, op_one, 1);
    chunk_add_byte(&chunk, op_constant, 1);
    chunk_add_byte(&chunk, (uint8_t)chunk_add_constant(&chunk, 3.14159265358979323846), 1);
    chunk_add_byte(&chunk, op_constant, 3);
    chunk_add_byte(&chunk, (uint8_t)chunk_add_constant(&chunk, 2.718281828459045), 3);
    chunk_add_byte(&chunk, op_multiply, 3);
    chunk_add_byte(&chunk, op_add, 1);
    chunk_add_byte(&chunk, op_return, 666);
#ifdef DEBUG
    chunk_debug(&chunk, "test");
#endif

    result r = vm_run(&vm, &chunk);
    chunk_free(&chunk);

    return r == result_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
