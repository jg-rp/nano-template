// SPDX-License-Identifier: MIT

#ifndef NT_EXPRESSION_H
#define NT_EXPRESSION_H

#include "nano_template/common.h"
#include "nano_template/context.h"
#include "nano_template/token.h"

/// @brief Possible expression kinds.
typedef enum
{
    EXPR_BOOL = 1,
    EXPR_NOT,
    EXPR_AND,
    EXPR_OR,
    EXPR_STR,
    EXPR_VAR
} NT_ExprKind;

/// @brief Internal expression type.
typedef struct NT_Expr
{
    // Child expressions, like the left and right hand side of the `or`
    // operator.
    struct NT_Expr *left;
    struct NT_Expr *right;

    // A string if EXPR_STR.
    // A list of path segments if EXPR_VAR.
    // NULL otherwise.
    PyObject *obj;

    // Optional token, used by EXPR_VAR to give the `Undefined` class line and
    // column numbers.
    NT_Token *token;

    NT_ExprKind kind;
} NT_Expr;

/// @brief Evaluate expression `self` with data from context `ctx`.
/// @return Arbitrary Python object, or NULL on failure.
PyObject *NT_Expression_evaluate(NT_Expr *self, NT_Context *ctx);

#endif