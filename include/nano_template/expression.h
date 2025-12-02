// SPDX-License-Identifier: MIT

#ifndef NT_EXPRESSION_H
#define NT_EXPRESSION_H

#include "nano_template/common.h"
#include "nano_template/context.h"
#include "nano_template/token.h"

#define NT_OBJ_PRE_PAGE 4

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

/// @brief One block of a paged array holding Python objects.
typedef struct NT_ObjPage
{
    struct NT_ObjPage *next;
    Py_ssize_t count;
    PyObject *objs[NT_OBJ_PRE_PAGE];
} NT_ObjPage;

/// @brief Internal expression type.
typedef struct NT_Expr
{
    // Child expressions, like the left and right hand side of the `or`
    // operator.
    struct NT_Expr *left;
    struct NT_Expr *right;

    // Paged array holding Python objects, like segments in a variable path.
    NT_ObjPage *head;
    NT_ObjPage *tail;

    // Optional token, used by EXPR_VAR to give the `Undefined` class line and
    // column numbers.
    NT_Token *token;

    NT_ExprKind kind;
} NT_Expr;

/// @brief Evaluate expression `self` with data from context `ctx`.
/// @return Arbitrary Python object, or NULL on failure.
PyObject *NT_Expression_evaluate(NT_Expr *self, NT_Context *ctx);

#endif