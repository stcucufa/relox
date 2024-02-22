#ifndef __LEXER_H__
#define __LEXER_H__

#include <stdlib.h>

typedef enum {
    token_begin,
    token_bang,
    token_quote,
    token_open_paren,
    token_close_paren,
    token_star,
    token_plus,
    token_comma,
    token_minus,
    token_period,
    token_slash,
    token_colon,
    token_semicolon,
    token_lt,
    token_equal,
    token_gt,
    token_open_brace,
    token_bar,
    token_close_brace,
    token_bang_equal,
    token_star_star,
    token_le,
    token_equal_equal,
    token_ge,
    token_infinity,
    token_string,
    token_string_prefix,
    token_string_infix,
    token_string_suffix,
    token_identifier,
    token_number,
    token_and,
    token_case,
    token_class,
    token_else,
    token_false,
    token_for,
    token_fun,
    token_if,
    token_let,
    token_nil,
    token_or,
    token_print,
    token_return,
    token_super,
    token_switch,
    token_this,
    token_true,
    token_var,
    token_while,
    token_error,
    token_eof,
    token_count,
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    size_t length;
    size_t line;
} Token;

#ifdef DEBUG
void token_debug(Token*);
#endif

typedef struct {
    const char* start;
    const char* current;
    size_t line;
    size_t string_nesting;
    TokenType last;
} Lexer;

void lexer_init(Lexer*, const char*);
Token lexer_advance(Lexer* lexer);

#endif
