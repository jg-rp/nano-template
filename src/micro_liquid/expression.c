#include "micro_liquid/expression.h"
#include "micro_liquid/py_token_view.h"

typedef PyObject *(*EvalFn)(ML_Expr *expr, ML_Context *ctx);

static PyObject *eval_bool_expr(ML_Expr *expr, ML_Context *ctx);
static PyObject *eval_not_expr(ML_Expr *expr, ML_Context *ctx);
static PyObject *eval_and_expr(ML_Expr *expr, ML_Context *ctx);
static PyObject *eval_or_expr(ML_Expr *expr, ML_Context *ctx);
static PyObject *eval_str_expr(ML_Expr *expr, ML_Context *ctx);
static PyObject *eval_var_expr(ML_Expr *expr, ML_Context *ctx);

static EvalFn eval_table[] = {
    [EXPR_BOOL] = eval_bool_expr, [EXPR_NOT] = eval_not_expr,
    [EXPR_AND] = eval_and_expr,   [EXPR_OR] = eval_or_expr,
    [EXPR_STR] = eval_str_expr,   [EXPR_VAR] = eval_var_expr,
};

static PyObject *undefined(PyObject **path, Py_ssize_t end_pos,
                           ML_Token *token, ML_Context *ctx);

ML_Expr *ML_Expression_new(ML_ExpressionKind kind, ML_Token *token,
                           ML_Expr **children, Py_ssize_t child_count,
                           PyObject *str, PyObject **path,
                           Py_ssize_t segment_count)
{
    ML_Expr *expr = PyMem_Malloc(sizeof(ML_Expr));

    if (!expr)
    {
        PyErr_NoMemory();
        return NULL;
    }

    expr->kind = kind;
    expr->token = token;
    expr->children = children;
    expr->child_count = child_count;
    expr->str = str;
    expr->path = path;
    expr->segment_count = segment_count;
    return expr;
}

void ML_Expression_dealloc(ML_Expr *self)
{
    if (!self)
    {
        return;
    }

    if (self->children)
    {
        for (Py_ssize_t i = 0; i < self->child_count; i++)
        {
            if (self->children[i])
            {
                ML_Expression_dealloc(self->children[i]);
            }
        }
        PyMem_Free(self->children);
    }

    if (self->token)
    {
        PyMem_Free(self->token);
    }

    Py_XDECREF(self->str);

    if (self->path)
    {
        for (Py_ssize_t i = 0; i < self->segment_count; i++)
        {
            Py_XDECREF(self->path[i]);
        }
        PyMem_Free(self->path);
    }

    PyMem_Free(self);
}

PyObject *ML_Expression_evaluate(ML_Expr *self, ML_Context *ctx)
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

static PyObject *eval_bool_expr(ML_Expr *expr, ML_Context *ctx)
{
    PyObject *op = NULL;
    PyObject *result = NULL;

    if (expr->child_count != 1)
    {
        result = Py_NewRef(Py_False);
        goto cleanup;
    }

    op = ML_Expression_evaluate(expr->children[0], ctx);
    if (!op)
    {
        goto cleanup;
    }

    int truthy = PyObject_IsTrue(op);

    if (truthy == 0)
    {
        result = Py_NewRef(Py_False);
    }
    else if (truthy == 1)
    {
        result = Py_NewRef(Py_True);
    }

cleanup:
    Py_XDECREF(op);
    return result;
}

static PyObject *eval_not_expr(ML_Expr *expr, ML_Context *ctx)
{
    PyObject *op = NULL;
    PyObject *result = NULL;

    if (!expr->children)
    {
        result = Py_NewRef(Py_True);
        goto cleanup;
    }

    op = ML_Expression_evaluate(expr->children[0], ctx);
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

static PyObject *eval_and_expr(ML_Expr *expr, ML_Context *ctx)
{
    PyObject *left = NULL;
    PyObject *right = NULL;
    PyObject *result = NULL;

    if (expr->child_count != 2)
    {
        result = Py_NewRef(Py_False);
        goto cleanup;
    }

    left = ML_Expression_evaluate(expr->children[0], ctx);
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

    right = ML_Expression_evaluate(expr->children[1], ctx);
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

static PyObject *eval_or_expr(ML_Expr *expr, ML_Context *ctx)
{
    PyObject *left = NULL;
    PyObject *right = NULL;
    PyObject *result = NULL;

    if (expr->child_count != 2)
    {
        result = Py_NewRef(Py_False);
        goto cleanup;
    }

    left = ML_Expression_evaluate(expr->children[0], ctx);

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

    right = ML_Expression_evaluate(expr->children[1], ctx);
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

static PyObject *eval_str_expr(ML_Expr *expr, ML_Context *ctx)
{
    (void)ctx;
    return Py_NewRef(expr->str);
}

static PyObject *eval_var_expr(ML_Expr *expr, ML_Context *ctx)
{
    Py_ssize_t count = expr->segment_count;
    PyObject **segments = expr->path;
    PyObject *op = NULL;
    PyObject *result = NULL;

    if (count == 0)
    {
        result = Py_NewRef(Py_None);
        goto cleanup;
    }

    if (ML_Context_get(ctx, segments[0], &op) < 0)
    {
        result = undefined(segments, 0, expr->token, ctx);
        goto cleanup;
    }

    if (count == 1)
    {
        result = Py_NewRef(op);
        goto cleanup;
    }

    for (Py_ssize_t i = 1; i < count; i++)
    {
        Py_DECREF(op);
        op = PyObject_GetItem(op, segments[i]);

        if (!op)
        {
            PyErr_Clear();
            result = undefined(segments, i, expr->token, ctx);
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
static PyObject *undefined(PyObject **path, Py_ssize_t end_pos,
                           ML_Token *token, ML_Context *ctx)
{
    PyObject *token_view = NULL;
    PyObject *list = NULL;
    PyObject *args = NULL;
    PyObject *item = NULL;
    PyObject *result = NULL;

    token_view =
        MLPY_TokenView_new(ctx->str, token->start, token->end, token->kind);

    if (!token_view)
    {
        goto cleanup;
    }

    list = PyList_New(end_pos + 1);
    if (!list)
    {
        goto cleanup;
    }

    for (Py_ssize_t i = 0; i <= end_pos; i++)
    {
        item = Py_NewRef(path[i]);

        if (PyList_SetItem(list, i, item) < 0)
        {
            // Undo the INCREF if insertion failed.
            Py_DECREF(item);
            goto cleanup;
        }
    }

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