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

typedef struct ML_Expr
{
    ML_ExpressionKind kind;
    ML_Token *token;
    struct ML_Expr **children;
    Py_ssize_t child_count;
    PyObject *str;            // Used by EXPR_STR
    PyObject **path;          // Used by EXPR_VAR
    Py_ssize_t segment_count; // Used by EXPR_VAR
} ML_Expr;

/**
 * Return a new expression.
 *
 * The new expression takes ownership of `token`, `children`, `str` and `path`,
 * which will be freed or DECREFed by `ML_Expression_destroy`.
 *
 * Pass NULL for `children` and/or `path` and `0` for `child_count` and/or
 * `segment_count` if the expression has no children or path.
 */
ML_Expr *ML_Expression_new(ML_ExpressionKind kind, ML_Token *token,
                           ML_Expr **children, Py_ssize_t child_count,
                           PyObject *str, PyObject **path,
                           Py_ssize_t segment_count);

void ML_Expression_destroy(ML_Expr *self);

/// Returns a new reference to the result of evaluating this expression in the
/// given context.
PyObject *ML_Expression_evaluate(ML_Expr *self, ML_Context *ctx);

#endif