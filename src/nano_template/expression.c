// SPDX-License-Identifier: MIT

#include "nano_template/expression.h"
#include "nano_template/py_token_view.h"

typedef PyObject *(*EvalFn)(NT_Expr *expr, NT_Context *ctx);

static PyObject *eval_not_expr(NT_Expr *expr, NT_Context *ctx);
static PyObject *eval_and_expr(NT_Expr *expr, NT_Context *ctx);
static PyObject *eval_or_expr(NT_Expr *expr, NT_Context *ctx);
static PyObject *eval_str_expr(NT_Expr *expr, NT_Context *ctx);
static PyObject *eval_var_expr(NT_Expr *expr, NT_Context *ctx);

static EvalFn eval_table[] = {
    [EXPR_NOT] = eval_not_expr, [EXPR_AND] = eval_and_expr,
    [EXPR_OR] = eval_or_expr,   [EXPR_STR] = eval_str_expr,
    [EXPR_VAR] = eval_var_expr,
};

static PyObject *undefined(NT_Expr *expr, NT_Context *ctx, Py_ssize_t end_pos);

PyObject *NT_Expression_evaluate(NT_Expr *self, NT_Context *ctx)
{
    if (!self)
    {
        return NULL;
    }

    EvalFn fn = eval_table[self->kind];

    if (!fn)
    {
        return NULL;
    }

    return fn(self, ctx);
}

static PyObject *eval_not_expr(NT_Expr *expr, NT_Context *ctx)
{
    PyObject *op = NULL;
    PyObject *result = NULL;

    if (!expr->right)
    {
        result = Py_NewRef(Py_True);
        goto cleanup;
    }

    op = NT_Expression_evaluate(expr->right, ctx);
    if (!op)
    {
        goto cleanup;
    }

    int falsy = PyObject_Not(op);

    if (falsy == 0)
    {
        result = Py_NewRef(Py_True);
    }
    else if (falsy == 1)
    {
        result = Py_NewRef(Py_False);
    }

cleanup:
    Py_XDECREF(op);
    return result;
}

static PyObject *eval_and_expr(NT_Expr *expr, NT_Context *ctx)
{
    PyObject *left = NULL;
    PyObject *right = NULL;
    PyObject *result = NULL;

    if (!expr->left || !expr->right)
    {
        result = Py_NewRef(Py_False);
        goto cleanup;
    }

    left = NT_Expression_evaluate(expr->left, ctx);
    if (!left)
    {
        goto cleanup;
    }

    int falsy = PyObject_Not(left);
    if (falsy == -1)
    {
        goto cleanup;
    }

    if (falsy)
    {
        // Short-circuit. Return left.
        result = Py_NewRef(left);
        goto cleanup;
    }

    right = NT_Expression_evaluate(expr->right, ctx);
    if (!right)
    {
        goto cleanup;
    }

    result = Py_NewRef(right);
    // Fall through.

cleanup:
    Py_XDECREF(left);
    Py_XDECREF(right);
    return result;
}

static PyObject *eval_or_expr(NT_Expr *expr, NT_Context *ctx)
{
    PyObject *left = NULL;
    PyObject *right = NULL;
    PyObject *result = NULL;

    if (!expr->left || !expr->right)
    {
        result = Py_NewRef(Py_False);
        goto cleanup;
    }

    left = NT_Expression_evaluate(expr->left, ctx);

    if (!left)
    {
        goto cleanup;
    }

    int truthy = PyObject_IsTrue(left);
    if (truthy == -1)
    {
        goto cleanup;
    }

    if (truthy)
    {
        // Short-circuit. Return left.
        result = Py_NewRef(left);
        goto cleanup;
    }

    right = NT_Expression_evaluate(expr->right, ctx);
    if (!right)
    {
        goto cleanup;
    }

    result = Py_NewRef(right);
    // Fall through.

cleanup:
    Py_XDECREF(left);
    Py_XDECREF(right);
    return result;
}

static PyObject *eval_str_expr(NT_Expr *expr, NT_Context *ctx)
{
    (void)ctx;
    return Py_NewRef(expr->obj);
}

static PyObject *eval_var_expr(NT_Expr *expr, NT_Context *ctx)
{
    PyObject *op = NULL;
    PyObject *result = NULL;
    PyObject *segment = NULL;
    PyObject *path = expr->obj;
    Py_ssize_t path_length = PyList_Size(path);

    if (path_length < 1)
    {
        return NULL;
    }

    segment = PyList_GetItem(path, 0);
    if (!segment)
    {
        PyErr_Clear();
        return NULL;
    }

    if (NT_Context_get(ctx, segment, &op) < 0)
    {
        result = undefined(expr, ctx, 0);
        goto cleanup;
    }

    if (path_length == 1)
    {
        result = Py_NewRef(op);
        goto cleanup;
    }

    for (Py_ssize_t i = 1; i < path_length; i++)
    {
        segment = PyList_GetItem(path, i);
        if (!segment)
        {
            PyErr_Clear();
            result = undefined(expr, ctx, i);
            goto cleanup;
        }

        Py_DECREF(op);
        op = PyObject_GetItem(op, segment);

        if (!op)
        {
            PyErr_Clear();
            result = undefined(expr, ctx, i);
            goto cleanup;
        }
    }

    result = Py_NewRef(op);

cleanup:
    Py_XDECREF(op);
    return result;
}

/// @brief Construct a new instance of Undefined.
/// @return A new Undefined object, or NULL on failure.
static PyObject *undefined(NT_Expr *expr, NT_Context *ctx, Py_ssize_t end_pos)
{
    PyObject *token_view = NULL;
    PyObject *segments = NULL;
    PyObject *args = NULL;
    PyObject *result = NULL;

    token_view = NTPY_TokenView_new(ctx->str, expr->token->start,
                                    expr->token->end, expr->token->kind);

    if (!token_view)
    {
        goto cleanup;
    }

    segments = PyList_GetSlice(expr->obj, 0, end_pos + 1);
    if (!segments)
    {
        goto cleanup;
    }

    args = Py_BuildValue("(O, O, O)", ctx->str, segments, token_view);
    if (!args)
    {
        goto cleanup;
    }

    result = PyObject_CallObject(ctx->undefined, args);
    // Fall through.

cleanup:
    Py_XDECREF(token_view);
    Py_XDECREF(segments);
    Py_XDECREF(args);
    return result;
}