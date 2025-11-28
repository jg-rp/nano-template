#ifndef ML_EXPRESSION_H
#define ML_EXPRESSION_H

#include "micro_liquid/common.h"
#include "micro_liquid/context.h"
#include "micro_liquid/token.h"

#define ML_OBJ_PRE_PAGE 4

/// @brief Possible expression kinds.
typedef enum
{
    EXPR_BOOL = 1,
    EXPR_NOT,
    EXPR_AND,
    EXPR_OR,
    EXPR_STR,
    EXPR_VAR
} ML_ExprKind;

/// @brief One block of a paged array holding Python objects.
typedef struct ML_ObjPage
{
    struct ML_ObjPage *next;
    Py_ssize_t count;
    PyObject *objs[ML_OBJ_PRE_PAGE];
} ML_ObjPage;

/// @brief Internal expression type.
typedef struct ML_Expr
{
    // Child expressions, like the left and right hand side of the `or`
    // operator.
    struct ML_Expr *left;
    struct ML_Expr *right;

    // Paged array holding Python objects, like segments in a variable path.
    ML_ObjPage *head;
    ML_ObjPage *tail;

    // Optional token, used by EXPR_VAR to give the `Undefined` class line and
    // column numbers.
    ML_Token *token;

    ML_ExprKind kind;
} ML_Expr;

/// @brief Evaluate expression `self` with data from context `ctx`.
/// @return Arbitrary Python object, or NULL on failure.
PyObject *ML_Expression_evaluate(ML_Expr *self, ML_Context *ctx);

#endif