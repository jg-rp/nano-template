// SPDX-License-Identifier: MIT

#include "nano_template/expression.h"
#include "nano_template/py_token_view.h"

typedef PyObject *(*EvalFn)(const NT_Expr *expr, NT_RenderContext *ctx);

static PyObject *eval_not_expr(const NT_Expr *expr, NT_RenderContext *ctx);
static PyObject *eval_and_expr(const NT_Expr *expr, NT_RenderContext *ctx);
static PyObject *eval_or_expr(const NT_Expr *expr, NT_RenderContext *ctx);
static PyObject *eval_str_expr(const NT_Expr *expr, NT_RenderContext *ctx);
static PyObject *eval_var_expr(const NT_Expr *expr, NT_RenderContext *ctx);

static EvalFn eval_table[] = {
    [EXPR_NOT] = eval_not_expr, [EXPR_AND] = eval_and_expr,
    [EXPR_OR] = eval_or_expr,   [EXPR_STR] = eval_str_expr,
    [EXPR_VAR] = eval_var_expr,
};

/// @brief Construct a new instance of Undefined.
/// @return A new Undefined object, or NULL on failure.
static PyObject *undefined(const NT_Expr *expr, NT_RenderContext *ctx,
                           size_t end_pos);

PyObject *NT_Expr_evaluate(const NT_Expr *expr, NT_RenderContext *ctx)
{
    if (!expr)
    {
        return NULL;
    }

    EvalFn fn = eval_table[expr->kind];

    if (!fn)
    {
        return NULL;
    }

    return fn(expr, ctx);
}

static PyObject *eval_not_expr(const NT_Expr *expr, NT_RenderContext *ctx)
{
    PyObject *op = NULL;
    PyObject *result = NULL;

    if (!expr->right)
    {
        result = Py_NewRef(Py_True);
        goto cleanup;
    }

    op = NT_Expr_evaluate(expr->right, ctx);
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

static PyObject *eval_and_expr(const NT_Expr *expr, NT_RenderContext *ctx)
{
    PyObject *left = NULL;
    PyObject *right = NULL;
    PyObject *result = NULL;

    if (!expr->left || !expr->right)
    {
        result = Py_NewRef(Py_False);
        goto cleanup;
    }

    left = NT_Expr_evaluate(expr->left, ctx);
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

    right = NT_Expr_evaluate(expr->right, ctx);
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

static PyObject *eval_or_expr(const NT_Expr *expr, NT_RenderContext *ctx)
{
    PyObject *left = NULL;
    PyObject *right = NULL;
    PyObject *result = NULL;

    if (!expr->left || !expr->right)
    {
        result = Py_NewRef(Py_False);
        goto cleanup;
    }

    left = NT_Expr_evaluate(expr->left, ctx);

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

    right = NT_Expr_evaluate(expr->right, ctx);
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

static PyObject *eval_str_expr(const NT_Expr *expr, NT_RenderContext *ctx)
{
    (void)ctx;
    if (!expr->head || expr->head->count < 1)
    {
        return NULL;
    }
    return Py_NewRef(expr->head->objs[0]);
}

static PyObject *eval_var_expr(const NT_Expr *expr, NT_RenderContext *ctx)
{
    PyObject *op = NULL;
    PyObject *result = NULL;

    if (!expr->head || expr->head->count == 0)
    {
        result = Py_NewRef(Py_None);
        goto cleanup;
    }

    if (NT_RenderContext_get(ctx, expr->head->objs[0], &op) < 0)
    {
        result = undefined(expr, ctx, 0);
        goto cleanup;
    }

    if (expr->head->count == 1)
    {
        result = Py_NewRef(op);
        goto cleanup;
    }

    NT_ObjPage *page = expr->head;
    while (page)
    {
        for (size_t i = 1; i < page->count; i++)
        {
            Py_DECREF(op);
            op = PyObject_GetItem(op, page->objs[i]);

            if (!op)
            {
                PyErr_Clear();
                result = undefined(expr, ctx, i);
                goto cleanup;
            }
        }

        page = page->next;
    }

    result = Py_NewRef(op);

cleanup:
    Py_XDECREF(op);
    return result;
}

static PyObject *undefined(const NT_Expr *expr, NT_RenderContext *ctx,
                           size_t end_pos)
{
    PyObject *token_view = NULL;
    PyObject *list = NULL;
    PyObject *args = NULL;
    PyObject *result = NULL;
    NT_ObjPage *page = NULL;

    token_view = NTPY_TokenView_new(ctx->str, expr->token->start,
                                    expr->token->end, expr->token->kind);

    if (!token_view)
    {
        goto cleanup;
    }

    list = PyList_New(0);
    if (!list)
    {
        goto cleanup;
    }

    size_t pos = 0;
    page = expr->head;

    while (page)
    {
        for (size_t i = 0; i <= page->count; i++)
        {
            if (PyList_Append(list, page->objs[i]) < 0)
            {
                goto cleanup;
            }

            if (pos >= end_pos)
            {
                goto end;
            }

            pos++;
        }

        page = page->next;
    }

end:
    args = Py_BuildValue("(O, O, O)", ctx->str, list, token_view);
    if (!args)
    {
        goto cleanup;
    }

    result = PyObject_CallObject(ctx->undefined, args);
    // Fall through.

cleanup:
    Py_XDECREF(token_view);
    Py_XDECREF(list);
    Py_XDECREF(args);
    return result;
}