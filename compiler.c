#include <stdlib.h>

#include "compiler.h"
#include "lexer.h"
#include "object.h"
#include "value.h"

struct Compiler;

typedef enum {
    precedence_eof,
    precedence_none,
    precedence_equality,
    precedence_inequality,
    precedence_addition,
    precedence_multiplication,
    precedence_exponentiation,
    precedence_unary,
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

static bool compiler_parse(Compiler* compiler, Precedence precedence) {
    Denotation nud = rules[compiler->current_token.type].nud;
    if (nud) {
        compiler_advance(compiler);
        nud(compiler);
        while (!compiler->error && rules[compiler->current_token.type].precedence > precedence) {
            compiler_advance(compiler);
            Denotation led = rules[compiler->previous_token.type].led;
            if (led) {
                led(compiler);
            } else {
                compiler_error(compiler, &compiler->previous_token, "expected no left operand");
            }
        }
    }
    return !compiler->error;
}

static void compiler_consume(Compiler* compiler, Token_Type type, const char* message) {
    if (compiler->current_token.type == type) {
        compiler_advance(compiler);
    } else {
        compiler_error(compiler, &compiler->current_token, message);
    }
}

static void compiler_emit_byte(Compiler* compiler, uint8_t byte) {
    chunk_add_byte(compiler->chunk, byte, compiler->lexer->line);
}

static void compiler_emit_constant(Compiler* compiler, Value value) {
    compiler_emit_byte(compiler, op_constant);
    compiler_emit_byte(compiler, (uint8_t)chunk_add_constant(compiler->chunk, value));
}

static void nud_group(Compiler* compiler) {
    if (compiler_parse(compiler, precedence_none)) {
        compiler_consume(compiler, token_close_paren, "expected `)`");
    }
}

static void nud_string(Compiler* compiler) {
    Token token = compiler->previous_token;
    Value string = VALUE_FROM_STRING(string_copy(token.start + 1, token.length - 2));
    vm_add_object(compiler->chunk->vm, string);
    compiler_emit_constant(compiler, string);
}

static void nud_number(Compiler* compiler) {
    double value = strtod(compiler->previous_token.start, 0);
    if (value == 0.0) {
        compiler_emit_byte(compiler, op_zero);
    } else if (value == 1.0) {
        compiler_emit_byte(compiler, op_one);
    } else {
        compiler_emit_constant(compiler, VALUE_FROM_NUMBER(value));
    }
}

static void nud_false(Compiler* compiler) {
    compiler_emit_byte(compiler, op_false);
}

static void nud_nil(Compiler* compiler) {
    compiler_emit_byte(compiler, op_nil);
}

static void nud_true(Compiler* compiler) {
    compiler_emit_byte(compiler, op_true);
}

static void nud_unary_op(Compiler* compiler) {
    uint8_t op = op_nop;
    switch (compiler->previous_token.type) {
        case token_bang: op = op_not; break;
        case token_minus: op = op_negate; break;
        default: break;
    }
    if (compiler_parse(compiler, precedence_unary)) {
        compiler_emit_byte(compiler, op);
    }
}

static void led_binary_op(Compiler* compiler) {
    Token_Type t = compiler->previous_token.type;
    uint8_t op = op_nop;
    switch (t) {
        case token_star: op = op_multiply; break;
        case token_plus: op = op_add; break;
        case token_minus: op = op_subtract; break;
        case token_slash: op = op_divide; break;
        case token_lt: op = op_lt; break;
        case token_gt: op = op_gt; break;
        case token_bang_equal: op = op_ne; break;
        case token_le: op = op_le; break;
        case token_equal_equal: op = op_eq; break;
        case token_ge: op = op_ge; break;
        default: break;
    }
    if (compiler_parse(compiler, rules[t].precedence)) {
        compiler_emit_byte(compiler, op);
    }
}

static void led_right_op(Compiler* compiler) {
    Token_Type t = compiler->previous_token.type;
    uint8_t op = op_nop;
    switch (t) {
        case token_star_star: op = op_exponent; break;
        default: break;
    }
    if (compiler_parse(compiler, rules[t].precedence - 1)) {
        compiler_emit_byte(compiler, op);
    }
}

Rule rules[] = {
    [token_bang] = { nud_unary_op, 0, precedence_none },
    [token_open_paren] = { nud_group, 0, precedence_none },
    [token_close_paren] = { 0, 0, precedence_none },
    [token_star] = { 0, led_binary_op, precedence_multiplication },
    [token_plus] = { nud_unary_op, led_binary_op, precedence_addition },
    [token_minus] = { nud_unary_op, led_binary_op, precedence_addition },
    [token_slash] = { 0, led_binary_op, precedence_multiplication },
    [token_lt] = { 0, led_binary_op, precedence_inequality },
    [token_gt] = { 0, led_binary_op, precedence_inequality },
    [token_bang_equal] = { 0, led_binary_op, precedence_equality },
    [token_le] = { 0, led_binary_op, precedence_inequality },
    [token_equal_equal] = { 0, led_binary_op, precedence_equality },
    [token_ge] = { 0, led_binary_op, precedence_inequality },
    [token_string] = { nud_string, 0, precedence_none },
    [token_star_star] = { 0, led_right_op, precedence_exponentiation },
    [token_number] = { nud_number, 0, precedence_none },
    [token_false] = { nud_false, 0, precedence_none },
    [token_nil] = { nud_nil, 0, precedence_none },
    [token_true] = { nud_true, 0, precedence_none },
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
    if (compiler_parse(&compiler, precedence_none)) {
        compiler_consume(&compiler, token_eof, "expected end of input");
        if (!compiler.error) {
            compiler_emit_byte(&compiler, op_return);
        }
    }
    return !compiler.error;
}
