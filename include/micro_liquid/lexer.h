#ifndef ML_Lexer_H
#define ML_Lexer_H

#include "micro_liquid/common.h"
#include "micro_liquid/token.h"

typedef enum
{
    STATE_MARKUP = 1,
    STATE_EXPR,
    STATE_TAG,
    STATE_OTHER,
    STATE_WC,
} ML_State;

typedef struct ML_Lexer
{
    PyObject *str;
    Py_ssize_t length;
    Py_ssize_t pos;
    ML_State *state;
    Py_ssize_t stack_size;
    Py_ssize_t stack_top;
} ML_Lexer;

ML_Lexer *ML_Lexer_new(PyObject *str);
void ML_Lexer_dealloc(ML_Lexer *self);

/// @brief Scan the next token.
/// @return The next token, or NULL on error with an exception set.
ML_Token *ML_Lexer_next(ML_Lexer *self);

/// @brief Scan all tokens.
/// @return An array of tokens with TOK_EOF as the last token, or NULL on
/// error.
ML_Token **ML_Lexer_scan(ML_Lexer *self, Py_ssize_t *out_token_count);

#endif
