// SPDX-License-Identifier: MIT

#ifndef NT_EXPRESSION_H
#define NT_EXPRESSION_H

#include "nano_template/common.h"
#include "nano_template/context.h"
#include "nano_template/token.h"

#define NT_OBJ_PRE_PAGE 4

typedef enum
{
    EXPR_BOOL = 1,
    EXPR_NOT,
    EXPR_AND,
    EXPR_OR,
    EXPR_STR,
    EXPR_VAR
} NT_ExprKind;

/// @brief One block of a paged array (unrolled linked list) holding Python
/// objects.
typedef struct NT_ObjPage
{
    struct NT_ObjPage *next;
    size_t count;
    PyObject *objs[NT_OBJ_PRE_PAGE];
} NT_ObjPage;

typedef struct NT_Expr
{
    // Child expressions, like the left and right hand side of the `or`
    // operator.
    struct NT_Expr *left;
    struct NT_Expr *right;

    // Paged array (unrolled linked list) holding Python objects, like segments
    // in a variable path.
    NT_ObjPage *head;
    NT_ObjPage *tail;

    // Optional token, used by EXPR_VAR to give the `Undefined` class line and
    // column numbers.
    NT_Token *token;

    NT_ExprKind kind;
} NT_Expr;

/// @brief Evaluate expression `expr` with data from context `ctx`.
/// @return Arbitrary Python object, or NULL on failure.
PyObject *NT_Expr_evaluate(const NT_Expr *expr, NT_RenderContext *ctx);

#endif