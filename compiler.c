#include <stdlib.h>

#include "compiler.h"
#include "hamt.h"
#include "lexer.h"
#include "object.h"
#include "value.h"

struct Compiler;

typedef enum {
    precedence_eof = -1,
    precedence_none = 0,
    precedence_interpolation,
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

Denotation statements[];
Rule rules[];

typedef struct Compiler {
    Chunk* chunk;
    HAMT constants;
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
static void compiler_emit_byte(Compiler*, uint8_t);

static bool compiler_match(Compiler* compiler, TokenType type) {
    if (compiler->current_token.type == type) {
        compiler_advance(compiler);
        return true;
    }
    return false;
}

static void compiler_consume(Compiler* compiler, TokenType type, const char* message) {
    if (compiler->current_token.type == type) {
        compiler_advance(compiler);
    } else {
        compiler_error(compiler, &compiler->current_token, message);
    }
}

static bool compiler_parse_expression(Compiler* compiler, Precedence precedence) {
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

static bool compiler_parse_statement(Compiler* compiler) {
    Denotation statement = statements[compiler->current_token.type];
    if (statement) {
        compiler_advance(compiler);
        statement(compiler);
    } else if (rules[compiler->current_token.type].nud) {
        if (compiler_parse_expression(compiler, precedence_none)) {
            compiler_emit_byte(compiler, op_pop);
        }
    } else {
        compiler_error(compiler, &compiler->current_token, "expected a statement");
    }
    if (!compiler->error) {
        compiler_consume(compiler, token_semicolon, "expected ; to end statement");
    }
    return !compiler->error;
}

static void compiler_emit_byte(Compiler* compiler, uint8_t byte) {
    chunk_add_byte(compiler->chunk, byte, compiler->lexer->line);
}

static void compiler_emit_bytes(Compiler* compiler, uint8_t x, uint8_t y) {
    chunk_add_byte(compiler->chunk, x, compiler->lexer->line);
    chunk_add_byte(compiler->chunk, y, compiler->lexer->line);
}

static void compiler_emit_constant(Compiler* compiler, Value value) {
    compiler_emit_byte(compiler, op_constant);
    uint8_t n = (uint8_t)chunk_add_constant(compiler->chunk, &compiler->constants, value);
    compiler_emit_byte(compiler, n);
}

static void compiler_string_constant(Compiler* compiler, Token* token) {
    size_t trim = token->type == token_string_prefix || token->type == token_string_infix ? 3 : 2;
    if (token->length == trim) {
        compiler_emit_byte(compiler, op_epsilon);
        return;
    }
    Value string = VALUE_FROM_STRING(string_copy(token->start + 1, token->length - trim));
#ifdef DEBUG
    fputs("\n", stderr);
#endif
    string = vm_add_object(compiler->chunk->vm, string);
    uint8_t n = chunk_add_constant(compiler->chunk, &compiler->constants, string);
    compiler_emit_bytes(compiler, op_constant, n);
}

static uint8_t compiler_symbol(Compiler* compiler, Token* token) {
    Value string = VALUE_FROM_STRING(string_copy(token->start, token->length));
#ifdef DEBUG
    fputs("\n", stderr);
#endif
    uint8_t n = vm_add_global(compiler->chunk->vm, string);
    return n;
}

static void compiler_string_interpolation(Compiler* compiler) {
    if (compiler_parse_expression(compiler, precedence_interpolation)) {
        if (compiler->current_token.type == token_string_infix ||
            compiler->current_token.type == token_string_suffix) {
            compiler_emit_byte(compiler, op_quote);
            compiler_emit_byte(compiler, op_multiply);
        } else {
            compiler_error(compiler, &compiler->current_token, "expected a continuing string");
        }
    }
}

static void statement_print(Compiler* compiler) {
    if (!rules[compiler->current_token.type].nud) {
        compiler_error(compiler, &compiler->current_token, "expected an expression");
    } else if (compiler_parse_expression(compiler, precedence_none)) {
        compiler_emit_byte(compiler, op_print);
    }
}

static void statement_var(Compiler* compiler) {
    compiler_consume(compiler, token_identifier, "expected variable name (identifier)");
    if (!compiler->error) {
        uint8_t n = compiler_symbol(compiler, &compiler->previous_token);
        if (compiler_match(compiler, token_equal)) {
            compiler_parse_expression(compiler, precedence_none);
        } else {
            compiler_emit_byte(compiler, op_nil);
        }
        compiler_emit_bytes(compiler, op_define_global, n);
    }
}

static void nud_group(Compiler* compiler) {
    if (compiler_parse_expression(compiler, precedence_none)) {
        compiler_consume(compiler, token_close_paren, "expected `)`");
    }
}

static void nud_bars(Compiler* compiler) {
    if (compiler_parse_expression(compiler, precedence_none)) {
        compiler_consume(compiler, token_bar, "expected `|`");
        compiler_emit_byte(compiler, op_bars);
    }
}

static void nud_string(Compiler* compiler) {
    compiler_string_constant(compiler, &compiler->previous_token);
}

static void nud_string_prefix(Compiler* compiler) {
    compiler_string_constant(compiler, &compiler->previous_token);
    compiler_string_interpolation(compiler);
}

static void led_string_suffix(Compiler* compiler) {
    compiler_string_constant(compiler, &compiler->previous_token);
    compiler_emit_byte(compiler, op_multiply);
    if (compiler->previous_token.type == token_string_infix) {
        compiler_string_interpolation(compiler);
    }
}

static void nud_identifier(Compiler* compiler) {
    uint8_t n = compiler_symbol(compiler, &compiler->previous_token);
    if (compiler_match(compiler, token_equal)) {
        compiler_parse_expression(compiler, precedence_none);
        compiler_emit_bytes(compiler, op_set_global, n);
    } else {
        compiler_emit_bytes(compiler, op_get_global, n);
    }
}

static void nud_number(Compiler* compiler) {
    if (compiler->previous_token.type == token_infinity) {
        compiler_emit_byte(compiler, op_infinity);
        return;
    }
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
        case token_quote: op = op_quote; break;
        case token_minus: op = op_negate; break;
        default: break;
    }
    if (compiler_parse_expression(compiler, precedence_unary)) {
        compiler_emit_byte(compiler, op);
    }
}

static void led_binary_op(Compiler* compiler) {
    TokenType t = compiler->previous_token.type;
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
    if (compiler_parse_expression(compiler, rules[t].precedence)) {
        compiler_emit_byte(compiler, op);
    }
}

static void led_right_op(Compiler* compiler) {
    TokenType t = compiler->previous_token.type;
    uint8_t op = op_nop;
    switch (t) {
        case token_star_star: op = op_exponent; break;
        default: break;
    }
    if (compiler_parse_expression(compiler, rules[t].precedence - 1)) {
        compiler_emit_byte(compiler, op);
    }
}

Denotation statements[] = {
    [token_print] = statement_print,
    [token_var] = statement_var,
};

Rule rules[] = {
    [token_bang] = { nud_unary_op, 0, precedence_none },
    [token_quote] = { nud_unary_op, 0, precedence_none },
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
    [token_bar] = { nud_bars, 0, precedence_none },
    [token_string] = { nud_string, 0, precedence_none },
    [token_string_prefix] = { nud_string_prefix, 0, precedence_none },
    [token_string_infix] = { 0, led_string_suffix, precedence_interpolation },
    [token_string_suffix] = { 0, led_string_suffix, precedence_interpolation },
    [token_star_star] = { 0, led_right_op, precedence_exponentiation },
    [token_infinity] = { nud_number, 0, precedence_none },
    [token_identifier] = { nud_identifier, 0, precedence_none },
    [token_number] = { nud_number, 0, precedence_none },
    [token_false] = { nud_false, 0, precedence_none },
    [token_nil] = { nud_nil, 0, precedence_none },
    [token_true] = { nud_true, 0, precedence_none },
    [token_eof] = { 0, 0, precedence_eof },
};

static void compiler_advance(Compiler* compiler) {
    compiler->previous_token = compiler->current_token;
    compiler->current_token = lexer_advance(compiler->lexer);

#ifdef DEBUG
    fprintf(stderr, ">>> [%zu] Current token: ", compiler->lexer->string_nesting);
    token_debug(&compiler->current_token);
#endif
}

bool compile_chunk(const char* source, Chunk* chunk) {
    Compiler compiler;
    Lexer lexer;
    lexer_init(&lexer, source);
    compiler.chunk = chunk;
    hamt_init(&compiler.constants);
    compiler.lexer = &lexer;
    compiler.error = false;
    compiler_advance(&compiler);
    do {
        compiler_parse_statement(&compiler);
    } while (!compiler.error && !compiler_match(&compiler, token_eof));
    compiler_emit_byte(&compiler, op_return);
    hamt_free(&compiler.constants);
    return !compiler.error;
}
