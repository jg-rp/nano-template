#ifndef ML_EXPRESSION_H
#define ML_EXPRESSION_H

#include "micro_liquid/common.h"
#include "micro_liquid/context.h"
#include "micro_liquid/token.h"

typedef enum
{
    EXPR_BOOL = 1,
    EXPR_NOT,
    EXPR_AND,
    EXPR_OR,
    EXPR_STR,
    EXPR_VAR
} ML_ExpressionKind;

// TODO: void *data instead of "Used by.."?

/// @brief Internal expression type.
/// `children`, `path` and `str` can all be NULL if the expression has no
/// children, path segments or associated string.
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

/// @brief Allocate and initialize a new ML_Expression.
/// Take/steal ownership of `token`, `children`, `str` and `path`.
/// @return Newly allocated ML_Expression*, or NULL on memory error.
ML_Expr *ML_Expression_new(ML_ExpressionKind kind, ML_Token *token,
                           ML_Expr **children, Py_ssize_t child_count,
                           PyObject *str, PyObject **path,
                           Py_ssize_t segment_count);

void ML_Expression_dealloc(ML_Expr *self);

/// @brief Evaluate expression `self` with data from context `ctx`.
/// @return Arbitrary Python object, or NULL on failure.
PyObject *ML_Expression_evaluate(ML_Expr *self, ML_Context *ctx);

#endif