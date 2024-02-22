#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

#ifdef DEBUG

char const*const tokens[token_count] = {
    [token_begin] = "begin",
    [token_bang] = "bang",
    [token_quote] = "quote",
    [token_open_paren] = "open paren",
    [token_close_paren] = "close paren",
    [token_star] = "star",
    [token_plus] = "plus",
    [token_comma] = "comma",
    [token_minus] = "minus",
    [token_period] = "period",
    [token_slash] = "slash",
    [token_colon] = "colon",
    [token_semicolon] = "semicolon",
    [token_lt] = "lt",
    [token_equal] = "equal",
    [token_gt] = "gt",
    [token_open_brace] = "open brace",
    [token_bar] = "bar",
    [token_close_brace] = "close brace",
    [token_bang_equal] = "bang + equal",
    [token_le] = "le",
    [token_equal_equal] = "equal + equal",
    [token_star_star] = "star + star",
    [token_ge] = "ge",
    [token_infinity] = "infinity",
    [token_string] = "string",
    [token_string_prefix] = "string prefix",
    [token_string_infix] = "string infix",
    [token_string_suffix] = "string suffix",
    [token_and] = "and",
    [token_case] = "case",
    [token_class] = "class",
    [token_default] = "default",
    [token_else] = "else",
    [token_false] = "false",
    [token_for] = "for",
    [token_fun] = "fun",
    [token_if] = "if",
    [token_let] = "let",
    [token_nil] = "nil",
    [token_or] = "or",
    [token_print] = "print",
    [token_return] = "return",
    [token_super] = "super",
    [token_switch] = "switch",
    [token_this] = "this",
    [token_true] = "true",
    [token_var] = "var",
    [token_while] = "while",
    [token_identifier] = "identifier",
    [token_number] = "number",
    [token_eof] = "EOF",
    [token_error] = "error",
};

void token_debug(Token* token) {
    fprintf(stderr, "`%.*s` (%s, line %zu)\n",
        (int)token->length, token->start, tokens[token->type], token->line);
}

#endif

static Token lexer_token(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = lexer->current - lexer->start;
    token.line = lexer->line;
    return token;
}

static Token lexer_error(Lexer* lexer, const char* message) {
    fprintf(stderr, "!!! Parse error, line %zu (near `%.*s`): %s\n",
        lexer->line, (int)(lexer->current - lexer->start), lexer->start, message);
    Token token;
    token.type = token_error;
    token.start = message;
    token.length = strlen(message);
    token.line = lexer->line;
    return token;
}

bool lexer_match(Lexer* lexer, char c) {
    if (*lexer->current != c) {
        return false;
    }
    lexer->current += 1;
    return true;
}

static void lexer_skip_whitespace_and_comments(Lexer* lexer) {
    for (;;) {
        switch (*lexer->current) {
            case '\n':
                lexer->line += 1;
                // fallthrough
            case ' ':
                lexer->current += 1;
                break;
            case '/':
                if (lexer->current[1] == '/') {
                    for (; *lexer->current != 0 && *lexer->current != '\n'; lexer->current += 1);
                    break;
                } else {
                    return;
                }
            default:
                return;
        }
    }
}

Token lexer_string(Lexer* lexer, char open) {
    for (;;) {
        switch (*lexer->current++) {
            case 0: return lexer_error(lexer, "unterminated string literal.");
            case '"':
                if (open == '"') {
                    return lexer_token(lexer, token_string);
                }
                lexer->string_nesting -= 1;
                return lexer_token(lexer, token_string_suffix);
            case '$':
                if (*lexer->current == '{') {
                    lexer->current += 1;
                    lexer->string_nesting += 1;
                    return lexer_token(
                        lexer,
                        open == '"' ? token_string_prefix : token_string_infix
                    );
                }
        }
    }
}

// ∞ (0x221e), UTF-8: 0xe2 0x88 0x9e
Token lexer_infinity(Lexer* lexer) {
    return ((unsigned char)*lexer->current++ == 0x88) && ((unsigned char)*lexer->current++ == 0x9e) ?
        lexer_token(lexer, token_infinity) : lexer_error(lexer, "unknown unicode character (expected ∞)");
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'A' && c <= 'Z') || c == '_' || (c >= 'a'&& c <= 'z');
}

static TokenType lexer_identifier_or_keyword(Lexer* lexer) {
#define KEYWORD(expected, length, type) \
    lexer->current - lexer->start == (length) && \
    strncmp(lexer->start, (expected), (length)) == 0 ? (type) : token_identifier

    switch (*lexer->start) {
        case 'a': return KEYWORD("and", 3, token_and);
        case 'c':
            switch (lexer->start[1]) {
                case 'a': return KEYWORD("case", 4, token_case);
                case 'l': return KEYWORD("class", 5, token_class);
            }
        case 'd': return KEYWORD("default", 7, token_default);
        case 'e': return KEYWORD("else", 4, token_else);
        case 'f':
            switch (lexer->start[1]) {
                case 'a': return KEYWORD("false", 5, token_false);
                case 'o': return KEYWORD("for", 3, token_for);
                case 'u': return KEYWORD("fun", 3, token_fun);
            }
            break;
        case 'i': return KEYWORD("if", 2, token_if);
        case 'l': return KEYWORD("let", 3, token_let);
        case 'n': return KEYWORD("nil", 3, token_nil);
        case 'o': return KEYWORD("or", 2, token_or);
        case 'p': return KEYWORD("print", 5, token_print);
        case 'r': return KEYWORD("return", 6, token_return);
        case 's':
            switch (lexer->start[1]) {
                case 'u': return KEYWORD("super", 5, token_super);
                case 'w': return KEYWORD("switch", 6, token_switch);
            }
            break;
        case 't':
            switch (lexer->start[1]) {
                case 'h': return KEYWORD("this", 4, token_this);
                case 'r': return KEYWORD("true", 4, token_true);
            }
            break;
        case 'v': return KEYWORD("var", 3, token_var);
        case 'w': return KEYWORD("while", 5, token_while);
    }
    return token_identifier;

#undef KEYWORD
}

Token lexer_advance(Lexer* lexer) {
#define MATCH(c) lexer_match(lexer, (c))
#define TOKEN(type) lexer_token(lexer, (type))

    lexer_skip_whitespace_and_comments(lexer);
    lexer->start = lexer->current;
    if (!*lexer->current) {
        return TOKEN(token_eof);
    }

    char c = *lexer->current++;
    switch ((unsigned char)c) {
        case '!': return TOKEN(MATCH('=') ? token_bang_equal : token_bang);
        case '"': return lexer_string(lexer, c);
        case '\'': return TOKEN(token_quote);
        case '(': return TOKEN(token_open_paren);
        case ')': return TOKEN(token_close_paren);
        case '*': return TOKEN(MATCH('*') ? token_star_star : token_star);
        case '+': return TOKEN(token_plus);
        case ',': return TOKEN(token_comma);
        case '-': return TOKEN(token_minus);
        case '.': return TOKEN(token_period);
        case '/': return TOKEN(token_slash);
        case ':': return TOKEN(token_colon);
        case ';': return TOKEN(token_semicolon);
        case '<': return TOKEN(MATCH('=') ? token_le : token_lt);
        case '=': return TOKEN(MATCH('=') ? token_equal_equal : token_equal);
        case '>': return TOKEN(MATCH('=') ? token_ge : token_gt);
        case '{': return TOKEN(token_open_brace);
        case '|': return TOKEN(token_bar);
        case '}': return lexer->string_nesting == 0 ? TOKEN(token_close_brace) : lexer_string(lexer, c);
        case 0xe2: return lexer_infinity(lexer);
    }

    if (is_digit(c)) {
        for (; is_digit(*lexer->current); lexer->current += 1);
        if (*lexer->current == '.') {
            do {
                lexer->current += 1;
            } while (is_digit(*lexer->current));
        }
        return TOKEN(token_number);
    }

    if (is_alpha(c)) {
        for (; is_alpha(*lexer->current) || is_digit(*lexer->current); lexer->current += 1);
        return TOKEN(lexer_identifier_or_keyword(lexer));
    }

    return lexer_error(lexer, "unexpected character.");

#undef MATCH
#undef TOKEN
}

void lexer_init(Lexer* lexer, const char* source) {
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->string_nesting = 0;
    lexer->last = token_begin;
}
