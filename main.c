#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "array.h"
#include "lexer.h"

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
    buffer.bytes[buffer.count - 1] = 0;
    char* output = (char*)malloc(buffer.count);
    memcpy(output, buffer.bytes, buffer.count);
    byte_array_free(&buffer);
    return output;
}

int main(int argc, char* argv[argc + 1]) {
    Lexer lexer;
    const char* source = argc == 1 || strcmp(argv[1], "-") == 0 ? read_stdin() : read_file(argv[1]);
    lexer_init(&lexer, source);
    while (lexer.last != token_error && lexer.last != token_eof) {
        Token token = lexer_advance(&lexer);
#ifdef DEBUG
        token_debug(token);
#endif
        lexer.last = token.type;
    }
    return lexer.last == token_eof ? EXIT_SUCCESS : EXIT_FAILURE;
}
