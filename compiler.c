#include <stdlib.h>

#include "compiler.h"
#include "lexer.h"

struct Compiler;

typedef enum {
    precedence_eof,
    precedence_none,
    precedence_addition,
    precedence_multiplication,
    precedence_exponentiation,
    precedence_unary,
    precedence_infinity,
} Precedence;

typedef void (*Denotation)(struct Compiler*);

typedef struct {
    Denotation nud;
    Denotation led;
    Precedence precedence;
} Rule;

Rule rules[];

typedef struct Compiler {
    Chunk* chunk;
    Lexer* lexer;
    Token previous_token;
    Token current_token;
    bool error;
} Compiler;

static void compiler_error(Compiler* compiler, Token* token, const char* message) {
    if (token->type != token_error) {
        fprintf(stderr, "!!! Compiler error, line %zu", token->line);
        if (token->type == token_eof) {
            fprintf(stderr, " (at end)");
        } else {
            fprintf(stderr, " (near `%.*s`)", (int)token->length, token->start);
        }
        fprintf(stderr, ": %s\n", message);
    }
    compiler->error = true;
}

static void compiler_advance(Compiler*);

static void compiler_parse(Compiler* compiler, Precedence precedence) {
    Denotation nud = rules[compiler->current_token.type].nud;
    if (!nud) {
        compiler_error(compiler, &compiler->current_token, "expected a left operand");
        return;
    }
    nud(compiler);
    if (compiler->error) {
        return;
    }
    while (precedence <= rules[compiler->current_token.type].precedence) {
        compiler_advance(compiler);
        Denotation led = rules[compiler->previous_token.type].led;
        if (!led) {
            compiler_error(compiler, &compiler->previous_token, "expected no left operand");
            return;
        }
        led(compiler);
        if (compiler->error) {
            return;
        }
    }
}

static void compiler_consume(Compiler* compiler, Token_Type type, const char* message) {
    if (compiler->current_token.type != type) {
        compiler_error(compiler, &compiler->current_token, message);
        return;
    }
    compiler_advance(compiler);
}

static void compiler_emit_byte(Compiler* compiler, uint8_t byte) {
    if (!compiler->error) {
        chunk_add_byte(compiler->chunk, byte, compiler->lexer->line);
    }
}

static void group(Compiler* compiler) {
    compiler_advance(compiler);
    compiler_parse(compiler, precedence_none);
    compiler_consume(compiler, token_close_paren, "expected `)`");
}

static void number(Compiler* compiler) {
    double value = strtod(compiler->lexer->start, 0);
    if (value == 0.0) {
        compiler_emit_byte(compiler, op_zero);
    } else if (value == 1.0) {
        compiler_emit_byte(compiler, op_one);
    } else {
        compiler_emit_byte(compiler, op_constant);
        compiler_emit_byte(compiler, (uint8_t)chunk_add_constant(compiler->chunk, value));
    }
    compiler_advance(compiler);
}

static void unary_op(Compiler* compiler) {
    uint8_t op = op_nop;
    switch (compiler->current_token.type) {
        case token_minus: op = op_negate; break;
        default: break;
    }
    compiler_advance(compiler);
    compiler_parse(compiler, precedence_unary);
    compiler_emit_byte(compiler, op);
}

static void binary_op(Compiler* compiler) {
    Token_Type t = compiler->previous_token.type;
    uint8_t op = op_nop;
    switch (t) {
        case token_star: op = op_multiply; break;
        case token_plus: op = op_add; break;
        case token_minus: op = op_subtract; break;
        case token_slash: op = op_divide; break;
        default: break;
    }
    compiler_parse(compiler, rules[t].precedence);
    compiler_emit_byte(compiler, op);
}

static void right_op(Compiler* compiler) {
    Token_Type t = compiler->previous_token.type;
    uint8_t op = op_nop;
    switch (t) {
        case token_star_star: op = op_exponent; break;
        default: break;
    }
    compiler_parse(compiler, rules[t].precedence - 1);
    compiler_emit_byte(compiler, op);
}

Rule rules[] = {
    [token_open_paren] = { group, 0, precedence_none },
    [token_star] = { 0, binary_op, precedence_multiplication },
    [token_plus] = { unary_op, binary_op, precedence_addition },
    [token_minus] = { unary_op, binary_op, precedence_addition },
    [token_slash] = { 0, binary_op, precedence_multiplication },
    [token_star_star] = { 0, right_op, precedence_exponentiation },
    [token_number] = { number, 0, precedence_none },
    [token_eof] = { 0, 0, precedence_eof },
};

static void compiler_advance(Compiler* compiler) {
    compiler->previous_token = compiler->current_token;
    compiler->current_token = lexer_advance(compiler->lexer);
}

bool compile_chunk(const char* source, Chunk* chunk) {
    Compiler compiler;
    Lexer lexer;
    lexer_init(&lexer, source);
    compiler.chunk = chunk;
    compiler.lexer = &lexer;
    compiler.error = false;
    compiler_advance(&compiler);
    compiler_parse(&compiler, precedence_none);
    if (!compiler.error) {
        compiler_consume(&compiler, token_eof, "expected end of input");
        compiler_emit_byte(&compiler, op_return);
    }
    return !compiler.error;
}
