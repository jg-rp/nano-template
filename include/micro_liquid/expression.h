#ifndef ML_EXPRESSION_H
#define ML_EXPRESSION_H

#include "micro_liquid/common.h"
#include "micro_liquid/context.h"
#include "micro_liquid/token.h"

/// @brief Possible expression kinds.
typedef enum
{
    EXPR_BOOL = 1,
    EXPR_NOT,
    EXPR_AND,
    EXPR_OR,
    EXPR_STR,
    EXPR_VAR
} ML_ExpressionKind;

/// @brief Internal expression type.
typedef struct ML_Expr
{
    ML_ExpressionKind kind;
    ML_Token *token;

    struct ML_Expr **children;
    Py_ssize_t child_count;
    Py_ssize_t child_capacity;

    PyObject **objects;
    Py_ssize_t object_count;
    Py_ssize_t object_capacity;
} ML_Expr;

/// @brief Allocate and initialize a new ML_Expression.
/// @return Newly allocated ML_Expression*, or NULL on memory error.
ML_Expr *ML_Expression_new(ML_ExpressionKind kind, ML_Token *token);

void ML_Expression_dealloc(ML_Expr *self);

/// @brief Evaluate expression `self` with data from context `ctx`.
/// @return Arbitrary Python object, or NULL on failure.
PyObject *ML_Expression_evaluate(ML_Expr *self, ML_Context *ctx);

/// @brief Append expression `expr` to `self`.
/// @return 0 on success, -1 on failure with an exception set.
int ML_Expression_add_child(ML_Expr *self, ML_Expr *expr);

/// @brief Append a Python object `obj` to `self`.
/// @return 0 on success, -1 on failure with an exception set.
int ML_Expression_add_obj(ML_Expr *self, PyObject *obj);

#endif