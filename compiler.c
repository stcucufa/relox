#include <stdlib.h>

#include "array.h"
#include "compiler.h"
#include "hamt.h"
#include "lexer.h"
#include "value.h"
#include "vm.h"

struct Compiler;

typedef enum {
    precedence_eof = -1,
    precedence_none = 0,
    precedence_interpolation,
    precedence_or,
    precedence_and,
    precedence_equality,
    precedence_inequality,
    precedence_addition,
    precedence_multiplication,
    precedence_exponentiation,
    precedence_call,
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
    Function* function;
    HAMT constants;
    ValueArray scopes;
    size_t locals_count;
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

static void compiler_emit_byte(Compiler* compiler, uint8_t byte) {
    chunk_add_byte(compiler->function->chunk, byte, compiler->lexer->line);
}

static void compiler_emit_bytes(Compiler* compiler, uint8_t x, uint8_t y) {
    chunk_add_byte(compiler->function->chunk, x, compiler->lexer->line);
    chunk_add_byte(compiler->function->chunk, y, compiler->lexer->line);
}

static void compiler_emit_constant(Compiler* compiler, Value value) {
    compiler_emit_byte(compiler, op_constant);
    uint8_t n = (uint8_t)chunk_add_constant(compiler->function->chunk, &compiler->constants, value);
    compiler_emit_byte(compiler, n);
}

static void compiler_emit_jump(Compiler* compiler, size_t to) {
    compiler_emit_byte(compiler, op_jump);
    ptrdiff_t offset = to - compiler->function->chunk->bytes.count - 2;
    compiler_emit_byte(compiler, (uint8_t)(offset >> 8));
    compiler_emit_byte(compiler, (uint8_t)offset);
}

static void compiler_advance(Compiler* compiler) {
    compiler->previous_token = compiler->current_token;
    compiler->current_token = lexer_advance(compiler->lexer);

#ifdef DEBUG
    fprintf(stderr, ">>> [%zu] Current token: ", compiler->lexer->string_nesting);
    token_debug(&compiler->current_token);
#endif
}

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
        compiler_consume(compiler, token_semicolon, "expected ; to end statement");
    } else {
        compiler_error(compiler, &compiler->current_token, "expected a statement");
    }
    return !compiler->error;
}

static void compiler_string_constant(Compiler* compiler, Token* token) {
    size_t trim = token->type == token_string_prefix || token->type == token_string_infix ? 3 : 2;
    if (token->length == trim) {
        compiler_emit_byte(compiler, op_epsilon);
        return;
    }
    Value string = value_copy_string(token->start + 1, token->length - trim);
    string = vm_add_object(compiler->function->chunk->vm, string);
    uint8_t n = chunk_add_constant(compiler->function->chunk, &compiler->constants, string);
    compiler_emit_bytes(compiler, op_constant, n);
}

static Var* compiler_declare_var(Compiler* compiler, Token* token, bool mutable) {
    Value string = value_copy_string(token->start, token->length);
    Value v = vm_add_object(compiler->function->chunk->vm, string);
    size_t i = compiler->scopes.count - 1;
    if (i == 0) {
        return vm_add_global(compiler->function->chunk->vm, v, mutable);
    }

    // We are in a local scope (there is more than one scope in the stack).
    HAMT* scope = VALUE_TO_HAMT(compiler->scopes.items[i]);
    Value w = hamt_get(scope, v);
    if (!VALUE_IS_NONE(w) && !VALUE_EQUAL(hamt_get(VALUE_TO_HAMT(compiler->scopes.items[i - 1]), v), w)) {
        compiler_error(compiler, token, "var is already defined");
        return 0;
    }

    if (compiler->locals_count == UINT8_MAX - 1) {
        compiler_error(compiler, token, "too many vars!");
        return 0;
    }

    Var* var = vm_var_new(compiler->function->chunk->vm, compiler->locals_count, mutable, false);
    compiler->locals_count += 1;
    scope = hamt_with(scope, v, VALUE_FROM_POINTER(var));
    scope = hamt_with(scope, VALUE_FROM_INT(var->index), v);
    compiler->scopes.items[i] = VALUE_FROM_POINTER(scope);
    return var;
}

static Var* compiler_find_var(Compiler* compiler, Token* token) {
    Value string = value_copy_string(token->start, token->length);
    Value v = vm_add_object(compiler->function->chunk->vm, string);
    size_t i = compiler->scopes.count - 1;
    if (i == 0) {
        return vm_add_global(compiler->function->chunk->vm, v, token->type == token_let);
    }
    Value w = hamt_get(VALUE_TO_HAMT(compiler->scopes.items[i]), v);
    if (VALUE_IS_NONE(w)) {
        compiler_error(compiler, token, "var is not defined");
        return 0;
    }
    return (Var*)VALUE_TO_POINTER(w);
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

static size_t compiler_stub_jump(Compiler* compiler, Opcode op) {
    compiler_emit_byte(compiler, op);
    compiler_emit_byte(compiler, 0xde);
    compiler_emit_byte(compiler, 0xad);
    return compiler->function->chunk->bytes.count;
}

static void compiler_patch_jump(Compiler* compiler, size_t dest) {
    ptrdiff_t offset = (compiler->function->chunk->bytes.count - dest);
    compiler->function->chunk->bytes.items[dest - 2] = (uint8_t)(offset >> 8);
    compiler->function->chunk->bytes.items[dest - 1] = (uint8_t)offset;
}

static size_t compiler_enter_scope(Compiler* compiler) {
    HAMT* parent_scope = VALUE_TO_HAMT(compiler->scopes.items[compiler->scopes.count - 1]);
    size_t parent_count = compiler->locals_count;
    value_array_push(&compiler->scopes, VALUE_FROM_POINTER(parent_scope));
    return parent_count;
}

static void compiler_exit_scope(Compiler* compiler, size_t parent_count) {
    for (; compiler->locals_count > parent_count; --compiler->locals_count) {
        compiler_emit_byte(compiler, op_pop);
    }
    HAMT* child_scope = VALUE_TO_HAMT(value_array_pop(&compiler->scopes));
    HAMT* parent_scope = VALUE_TO_HAMT(compiler->scopes.items[compiler->scopes.count - 1]);
    if (child_scope != parent_scope) {
        hamt_free(child_scope);
    }
}

static void statement_block(Compiler* compiler) {
    size_t parent_count = compiler_enter_scope(compiler);
    while (!compiler->error && compiler->current_token.type != token_close_brace) {
        compiler_parse_statement(compiler);
    }
    if (!compiler->error) {
        compiler_consume(compiler, token_close_brace, "expected } to end a block");
    }
    compiler_exit_scope(compiler, parent_count);
}

// if <predicate-expr> <block-statement>
// if <predicate-expr> <block-statement> else <block-statement>
static void statement_if(Compiler* compiler) {
    compiler_parse_expression(compiler, precedence_none);
    if (!compiler->error) {
        size_t jump_over_consequent = compiler_stub_jump(compiler, op_jump_false);
        compiler_emit_byte(compiler, op_pop);
        compiler_consume(compiler, token_open_brace, "expected { after if and predicate");
        statement_block(compiler);
        if (!compiler->error && compiler_match(compiler, token_else)) {
            compiler_consume(compiler, token_open_brace, "expected { after else");
            size_t jump_over_alternate = compiler_stub_jump(compiler, op_jump);
            compiler_patch_jump(compiler, jump_over_consequent);
            compiler_emit_byte(compiler, op_pop);
            statement_block(compiler);
            compiler_patch_jump(compiler, jump_over_alternate);
        } else {
            compiler_patch_jump(compiler, jump_over_consequent);
        }
    }
}

// switch <expr> {
//     case <expr>: <statements>
//     case <expr>: <statements> [fallthrough;]
//     ...
//     [default: <statements>]
// }
static void statement_switch(Compiler* compiler) {
    compiler_parse_expression(compiler, precedence_none);
    compiler_consume(compiler, token_open_brace, "expected { after switch expression");

    // Keep track of the break jumps for every case to patch them later.
    ValueArray breaks;
    value_array_init(&breaks);

    size_t skip, fallthrough = 0;
    bool did_fallthrough = false;
    while (compiler_match(compiler, token_case)) {
        if (breaks.count > 0) {
            compiler_emit_byte(compiler, op_pop);
        }
        compiler_emit_byte(compiler, op_dup);
        compiler_parse_expression(compiler, precedence_none);
        compiler_consume(compiler, token_colon, "expected : after case");
        compiler_emit_byte(compiler, op_eq);
        skip = compiler_stub_jump(compiler, op_jump_false);
        compiler_emit_bytes(compiler, op_pop, op_pop);
        if (did_fallthrough) {
            did_fallthrough = false;
            compiler_patch_jump(compiler, fallthrough);
        }
        do {
            compiler_parse_statement(compiler);
        } while (!compiler->error &&
            compiler->current_token.type != token_case &&
            compiler->current_token.type != token_default &&
            compiler->current_token.type != token_fallthrough &&
            compiler->current_token.type != token_close_brace);
        if (compiler_match(compiler, token_fallthrough)) {
            compiler_consume(compiler, token_semicolon, "expected ; after fallthrough");
            did_fallthrough = true;
        } else {
            size_t jump = compiler_stub_jump(compiler, op_jump);
            value_array_push(&breaks, VALUE_FROM_INT(jump));
        }
        compiler_patch_jump(compiler, skip);
        if (did_fallthrough) {
            fallthrough = compiler_stub_jump(compiler, op_jump);
        }
    }

    if (compiler_match(compiler, token_default)) {
        if (breaks.count > 0) {
            compiler_emit_byte(compiler, op_pop);
        }
        compiler_consume(compiler, token_colon, "expected : after default");
        if (did_fallthrough) {
            did_fallthrough = false;
            compiler_patch_jump(compiler, fallthrough);
        }
        do {
            compiler_parse_statement(compiler);
        } while (!compiler->error && compiler->current_token.type != token_close_brace);
    }

    if (did_fallthrough) {
        did_fallthrough = false;
        compiler_patch_jump(compiler, fallthrough);
    }

    for (size_t i = 0; i < breaks.count; ++i) {
        compiler_patch_jump(compiler, VALUE_TO_INT(breaks.items[i]));
    }
    value_array_free(&breaks);
    compiler_consume(compiler, token_close_brace, "expected } to close switch statement");
}

// while <predicate-expr> <block-statement>
static void statement_while(Compiler* compiler) {
    size_t predicate = compiler->function->chunk->bytes.count;
    compiler_parse_expression(compiler, precedence_none);
    if (!compiler->error) {
        size_t jump = compiler_stub_jump(compiler, op_jump_false);
        compiler_emit_byte(compiler, op_pop);
        compiler_consume(compiler, token_open_brace, "expected { after while predicate");
        statement_block(compiler);
        compiler_emit_jump(compiler, predicate);
        compiler_patch_jump(compiler, jump);
    }
}

// var <identifier> ;
// var <identifier> = <expression> ;
// let <identifier> ;
// let <identifier> = <expression> ;
static void statement_declaration(Compiler* compiler) {
    bool mutable = compiler->previous_token.type == token_var;
    compiler_consume(compiler, token_identifier, "expected variable name (identifier)");
    if (!compiler->error) {
        Var* var = compiler_declare_var(compiler, &compiler->previous_token, mutable);
        var->initialized = true;
        if (compiler_match(compiler, token_equal)) {
            compiler_parse_expression(compiler, precedence_none);
        } else {
            compiler_emit_byte(compiler, op_nil);
        }
        compiler_consume(compiler, token_semicolon, "expected ; to end var statement");
        if (compiler->scopes.count == 1) {
            compiler_emit_bytes(compiler, op_define_global, var->index);
        }
    }
}

// fun <identifier> ( ) { <block> }
// fun foo() { print "ok"; }
static void statement_function_declaration(Compiler* compiler) {
    compiler_consume(compiler, token_identifier, "expected function name");
    if (compiler->error) {
        return;
    }
    Token* name_token = &compiler->previous_token;
    Var* var = compiler_declare_var(compiler, name_token, false);
    var->initialized = true;

    Function* outerFunction = compiler->function;
    compiler->function = function_new();
    compiler->function->name = value_copy_string(name_token->start, name_token->length);
    compiler->function->chunk->vm = outerFunction->chunk->vm;
    size_t parent_count = compiler_enter_scope(compiler);
    compiler->locals_count += 1;

    compiler_consume(compiler, token_open_paren, "expected ( after function name");
    if (compiler->current_token.type != token_close_paren) {
        do {
            compiler_consume(compiler, token_identifier, "expected a parameter name");
            compiler_declare_var(compiler, &compiler->previous_token, true)->initialized = true;
            compiler->function->arity += 1;
        } while (compiler_match(compiler, token_comma));
    }
    compiler_consume(compiler, token_close_paren, "expected ) after function parameters");
    compiler_consume(compiler, token_open_brace, "expected { before function body");
    statement_block(compiler);

    if (compiler->function->chunk->bytes.items[compiler->function->chunk->bytes.count - 1] != op_return) {
        compiler_emit_bytes(compiler, op_nil, op_return);
    }
    Value function = VALUE_FROM_FUNCTION(compiler->function);

#ifdef DEBUG
    chunk_debug(compiler->function->chunk, value_to_cstring(compiler->function->name));
#endif

    compiler_exit_scope(compiler, parent_count);
    compiler->function = outerFunction;
    uint8_t n = chunk_add_constant(compiler->function->chunk, &compiler->constants, function);
    compiler_emit_bytes(compiler, op_constant, n);
    if (compiler->scopes.count == 1) {
        compiler_emit_bytes(compiler, op_define_global, var->index);
    }
}

// for ( <expr> | <declaration>; <predicate-expr>; <increment-expr> ) <block-statement>
static void statement_for(Compiler* compiler) {
    compiler_consume(compiler, token_open_paren, "expected ( after for");

    if (compiler_match(compiler, token_var)) {
        statement_declaration(compiler);
    } else {
        compiler_parse_expression(compiler, precedence_none);
        compiler_consume(compiler, token_semicolon, "expected ; after initialization part of for");
    }

    size_t predicate = compiler->function->chunk->bytes.count;
    compiler_parse_expression(compiler, precedence_none);
    compiler_consume(compiler, token_semicolon, "expected ; after predicate part of for");
    size_t exit_jump = compiler_stub_jump(compiler, op_jump_false);
    compiler_emit_byte(compiler, op_pop);
    size_t body_jump = compiler_stub_jump(compiler, op_jump);

    compiler_parse_expression(compiler, precedence_none);
    compiler_emit_byte(compiler, op_pop);
    compiler_consume(compiler, token_close_paren, "expected ) after increment part of for");
    compiler_emit_jump(compiler, predicate);

    compiler_patch_jump(compiler, body_jump);
    compiler_consume(compiler, token_open_brace, "expected { after while predicate");
    statement_block(compiler);
    compiler_emit_jump(compiler, body_jump);
    compiler_patch_jump(compiler, exit_jump);
    compiler_emit_byte(compiler, op_pop);
}

// return ;
// return <expression> ;
static void statement_return(Compiler* compiler) {
    if (compiler_match(compiler, token_semicolon)) {
        compiler_emit_bytes(compiler, op_nil, op_return);
    } else {
        if (compiler->scopes.count == 1) {
            compiler_error(compiler, &compiler->current_token, "Cannot return a value from a script");
        }
        compiler_parse_expression(compiler, precedence_none);
        compiler_consume(compiler, token_semicolon, "expected ; to end print statement");
        compiler_emit_byte(compiler, op_return);
    }
}

// print <expression> ;
static void statement_print(Compiler* compiler) {
    if (!rules[compiler->current_token.type].nud) {
        compiler_error(compiler, &compiler->current_token, "expected an expression");
    } else if (compiler_parse_expression(compiler, precedence_none)) {
        compiler_consume(compiler, token_semicolon, "expected ; to end print statement");
        compiler_emit_byte(compiler, op_print);
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
    Var* var = compiler_find_var(compiler, &compiler->previous_token);
    if (compiler_match(compiler, token_equal)) {
        compiler_parse_expression(compiler, precedence_none);
        if (var->initialized && !var->mutable) {
            compiler_error(compiler, &compiler->previous_token, "let binding is not mutable");
            return;
        }
        compiler_emit_bytes(compiler, var->global ? op_set_global : op_set_local, var->index);
    } else {
        compiler_emit_bytes(compiler, var->global ? op_get_global : op_get_local, var->index);
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

static void led_call(Compiler* compiler) {
    uint8_t arg_count = 0;
    if (compiler->current_token.type != token_close_paren) {
        do {
            compiler_parse_expression(compiler, precedence_none);
            arg_count += 1;
        } while (compiler_match(compiler, token_comma));
    }
    compiler_consume(compiler, token_close_paren, "expected ) after function arguments");
    compiler_emit_bytes(compiler, op_call, arg_count);
}

static void led_and_or(Compiler* compiler) {
    TokenType t = compiler->previous_token.type;
    size_t jump = compiler_stub_jump(compiler, t == token_or ? op_jump_true : op_jump_false);
    compiler_emit_byte(compiler, op_pop);
    compiler_parse_expression(compiler, rules[t].precedence);
    compiler_patch_jump(compiler, jump);
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
    [token_for] = statement_for,
    [token_fun] = statement_function_declaration,
    [token_if] = statement_if,
    [token_let] = statement_declaration,
    [token_open_brace] = statement_block,
    [token_print] = statement_print,
    [token_return] = statement_return,
    [token_var] = statement_declaration,
    [token_switch] = statement_switch,
    [token_while] = statement_while,
};

Rule rules[] = {
    [token_bang] = { nud_unary_op, 0, precedence_none },
    [token_quote] = { nud_unary_op, 0, precedence_none },
    [token_open_paren] = { nud_group, led_call, precedence_call },
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
    [token_and] = { 0, led_and_or, precedence_and },
    [token_number] = { nud_number, 0, precedence_none },
    [token_or] = { 0, led_and_or, precedence_or },
    [token_false] = { nud_false, 0, precedence_none },
    [token_nil] = { nud_nil, 0, precedence_none },
    [token_true] = { nud_true, 0, precedence_none },
    [token_eof] = { 0, 0, precedence_eof },
};

bool compile_function(const char* source, Function* function) {
    Compiler compiler;
    Lexer lexer;
    lexer_init(&lexer, source);
    compiler.function = function;
    hamt_init(&compiler.constants);
    value_array_init(&compiler.scopes);
    value_array_push(&compiler.scopes, VALUE_FROM_POINTER(&function->chunk->vm->global_scope));
    compiler.locals_count = 0;
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
