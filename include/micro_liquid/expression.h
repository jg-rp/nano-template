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

typedef struct ML_Expression
{
    ML_ExpressionKind kind;
    ML_Token *token;
    struct ML_Expression **children;

} ML_Expression;

/**
 * Return a new expression.
 *
 * The new expression takes ownership of `token` and `children`, which will be
 * freed by `ML_Expression_destroy`.
 */
ML_Expression *ML_Expression_new(ML_ExpressionKind kind, ML_Token *token,
                                 ML_Expression **children);

void ML_Expression_destroy(ML_Expression *self);

PyObject *ML_Expression_evaluate(ML_Expression *self, ML_Context *ctx);

#endif