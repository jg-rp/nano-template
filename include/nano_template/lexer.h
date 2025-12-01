#ifndef NT_Lexer_H
#define NT_Lexer_H

#include "nano_template/allocator.h"
#include "nano_template/common.h"
#include "nano_template/token.h"

typedef enum
{
    STATE_MARKUP = 1,
    STATE_EXPR,
    STATE_TAG,
    STATE_OTHER,
    STATE_WC,
} NT_State;

typedef struct NT_Lexer
{
    PyObject *str;
    Py_ssize_t length;
    Py_ssize_t pos;
    NT_State *state;
    Py_ssize_t stack_size;
    Py_ssize_t stack_top;
} NT_Lexer;

NT_Lexer *NT_Lexer_new(PyObject *str);
void NT_Lexer_free(NT_Lexer *l);

/// @brief Scan the next token.
/// @return The next token, or NULL on error with an exception set.
NT_Token NT_Lexer_next(NT_Lexer *l);

/// @brief Scan all tokens.
/// @return An array of tokens with TOK_EOF as the last token, or NULL on
/// error.
NT_Token *NT_Lexer_scan(NT_Lexer *l, Py_ssize_t *out_token_count);

#endif
