#ifndef ML_Lexer_H
#define ML_Lexer_H

#include "micro_liquid/common.h"
#include "micro_liquid/lexer_state.h"
#include "micro_liquid/token.h"

typedef struct ML_Lexer
{
    PyObject *str;
    Py_ssize_t pos;
    ML_StateStack state;
} ML_Lexer;

ML_Lexer *ML_Lexer_new(PyObject *str);
void ML_Lexer_destroy(ML_Lexer *self);

/**
 * Return the next token, or TOK_EOF if we've reached the end of the input
 * string.
 *
 * On error, set a Python exception and return NULL.
 */
ML_Token *ML_Lexer_next(ML_Lexer *self);

/**
 * Return all tokens as a caller owned array of `ML_Token` pointers.
 * The caller should free `ML_Lexer` with `ML_Lexer_destroy` after calling this
 * function.
 */
ML_Token **ML_Lexer_scan(ML_Lexer *self, Py_ssize_t *out_token_count);

#endif
