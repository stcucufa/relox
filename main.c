#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "vm.h"

static const char* read_file(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Could not open \"%s\" for reading. ", path);
        perror(0);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    int length = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(length + 1);
    size_t bytes_read = fread(buffer, 1, length, file);
    if (bytes_read != length) {
        fprintf(stderr, "Could not read all contents from \"%s\". ", path);
        perror(0);
        exit(EXIT_FAILURE);
    }
    buffer[length] = 0;
    fclose(file);
    return buffer;
}

static const char* read_stdin(void) {
    ByteArray buffer;
    byte_array_init(&buffer);
    while (!feof(stdin)) {
        byte_array_push(&buffer, getchar());
    }
    buffer.items[buffer.count - 1] = 0;
    char* output = (char*)malloc(buffer.count);
    memcpy(output, buffer.items, buffer.count);
    byte_array_free(&buffer);
    return output;
}

int main(int argc, char* argv[argc + 1]) {
    VM vm;
    const char* source = argc == 1 || strcmp(argv[1], "-") == 0 ? read_stdin() : read_file(argv[1]);
    Result result = vm_compile_and_run(&vm, source);
    vm_free(&vm);
    return result == result_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
