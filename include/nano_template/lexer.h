// SPDX-License-Identifier: MIT

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
    PyObject *str;     // String to scan.
    Py_ssize_t length; // Length of str.
    Py_ssize_t pos;    // Current index into str.

    NT_State *state; // A stack of lexer states.
    Py_ssize_t stack_capacity;
    Py_ssize_t stack_top;
} NT_Lexer;

/// @brief Allocate and initialize a new NT_Lexer.
/// @return A pointer to the new lexer, or NULL on failure with an exception
/// set.
NT_Lexer *NT_Lexer_new(PyObject *str);

void NT_Lexer_free(NT_Lexer *l);

/// @brief Scan the next token.
/// @return The next token, or NULL on error with an exception set.
NT_Token NT_Lexer_next(NT_Lexer *l);

/// @brief Scan all tokens.
/// @return A new array of tokens with TOK_EOF as the last token, or NULL on
/// error.
NT_Token *NT_Lexer_scan(NT_Lexer *l, Py_ssize_t *out_token_count);

#endif
