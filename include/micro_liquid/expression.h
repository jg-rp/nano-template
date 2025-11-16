#ifndef ML_EXPRESSION_H
#define ML_EXPRESSION_H

#include "micro_liquid/common.h"
#include "micro_liquid/context.h"
#include "micro_liquid/token.h"

typedef enum
{
    EXPR_BOOL,
    EXPR_NOT,
    EXPR_AND,
    EXPR_OR,
    EXPR_STR,
    EXPR_VAR
} ML_ExpressionKind;

// TODO: void *data instead of "Used by.."?

typedef struct ML_Expression
{
    ML_ExpressionKind kind;
    ML_Token *token;
    struct ML_Expression **children;
    Py_ssize_t child_count;
    PyObject *str;            // Used by EXPR_STR
    PyObject **path;          // Used by EXPR_VAR
    Py_ssize_t segment_count; // Used by EXPR_VAR
} ML_Expression;

/**
 * Return a new expression.
 *
 * The new expression takes ownership of `token`, `children`, `str` and `path`,
 * which will be freed by `ML_Expression_destroy`.
 *
 * Pass NULL for `children` and/or `path` and `0` for `child_count` and/or
 * `segment_count` if the expression has no children or path.
 */
ML_Expression *ML_Expression_new(ML_ExpressionKind kind, ML_Token *token,
                                 ML_Expression **children,
                                 Py_ssize_t child_count, PyObject *str,
                                 PyObject **path, Py_ssize_t segment_count);

void ML_Expression_destroy(ML_Expression *self);

PyObject *ML_Expression_evaluate(ML_Expression *self, ML_Context *ctx);

#endif