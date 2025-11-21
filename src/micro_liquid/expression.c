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
    if (!expr->children)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    PyObject *op = ML_Expression_evaluate(expr->children[0], ctx);
    if (!op)
    {
        return NULL;
    }

    int truthy = PyObject_IsTrue(op);
    Py_XDECREF(op);

    switch (truthy)
    {
    case 0:
        Py_INCREF(Py_False);
        return Py_False;
    case 1:
        Py_INCREF(Py_True);
        return Py_True;
    default:
        return NULL;
    }
}

static PyObject *eval_not_expr(ML_Expr *expr, ML_Context *ctx)
{
    if (!expr->children)
    {
        Py_INCREF(Py_True);
        return Py_True;
    }

    PyObject *op = ML_Expression_evaluate(expr->children[0], ctx);
    if (!op)
    {
        return NULL;
    }

    int falsy = PyObject_Not(op);
    Py_XDECREF(op);

    switch (falsy)
    {
    case 0:
        Py_INCREF(Py_True);
        return Py_True;
    case 1:
        Py_INCREF(Py_False);
        return Py_False;
    default:
        return NULL;
    }
}

static PyObject *eval_and_expr(ML_Expr *expr, ML_Context *ctx)
{
    if (expr->child_count != 2)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    PyObject *op_left = ML_Expression_evaluate(expr->children[0], ctx);
    PyObject *op_right = NULL;

    if (!op_left)
    {
        goto fail;
    }

    int falsy = PyObject_Not(op_left);
    if (falsy == -1)
    {
        goto fail;
    }

    if (falsy)
    {
        return op_left;
    }

    op_right = ML_Expression_evaluate(expr->children[1], ctx);
    if (!op_right)
    {
        goto fail;
    }

    Py_XDECREF(op_left);
    return op_right;

fail:
    Py_XDECREF(op_left);
    Py_XDECREF(op_right);
    return NULL;
}

static PyObject *eval_or_expr(ML_Expr *expr, ML_Context *ctx)
{
    if (expr->child_count != 2)
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    PyObject *op_left = ML_Expression_evaluate(expr->children[0], ctx);
    PyObject *op_right = NULL;

    if (!op_left)
    {
        goto fail;
    }

    int truthy = PyObject_IsTrue(op_left);
    if (truthy == -1)
    {
        goto fail;
    }

    if (truthy)
    {
        return op_left;
    }

    op_right = ML_Expression_evaluate(expr->children[1], ctx);
    if (!op_right)
    {
        goto fail;
    }

    Py_XDECREF(op_left);
    return op_right;

fail:
    Py_XDECREF(op_left);
    Py_XDECREF(op_right);
    return NULL;
}

static PyObject *eval_str_expr(ML_Expr *expr, ML_Context *ctx)
{
    (void)ctx;
    Py_INCREF(expr->str);
    return expr->str;
}

static PyObject *eval_var_expr(ML_Expr *expr, ML_Context *ctx)
{
    Py_ssize_t count = expr->segment_count;
    PyObject **segments = expr->path;
    PyObject *op = NULL;

    if (count == 0)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (ML_Context_get(ctx, segments[0], &op) < 0)
    {
        return undefined(segments, 0, expr->token, ctx);
    }

    if (count == 1)
    {
        return op;
    }

    for (Py_ssize_t i = 1; i < count; i++)
    {
        Py_DECREF(op);
        op = PyObject_GetItem(op, segments[i]);

        if (!op)
        {
            PyErr_Clear();
            return undefined(segments, i, expr->token, ctx);
        }
    }

    return op;
}

/// @brief Construct a new instance of Undefined.
/// @return A new Undefined object, or NULL on failure.
static PyObject *undefined(PyObject **path, Py_ssize_t end_pos,
                           ML_Token *token, ML_Context *ctx)
{
    PyObject *list = NULL;
    PyObject *args = NULL;

    PyObject *token_view =
        MLPY_TokenView_new(ctx->str, token->start, token->end, token->kind);

    if (!token_view)
    {
        return NULL;
    }

    list = PyList_New(end_pos + 1);
    if (!list)
    {
        goto fail;
    }

    for (Py_ssize_t i = 0; i <= end_pos; i++)
    {
        PyObject *item = path[i];
        Py_INCREF(item);

        if (PyList_SetItem(list, i, item) < 0)
        {
            // Undo the INCREF if insertion failed.
            Py_DECREF(item);
            goto fail;
        }
    }

    args = Py_BuildValue("(O, O)", list, token_view);
    PyObject *undef = PyObject_CallObject(ctx->undefined, args);
    if (!undef)
    {
        goto fail;
    }

    Py_XDECREF(token_view);
    Py_XDECREF(list);
    Py_XDECREF(args);
    return undef;

fail:
    Py_XDECREF(token_view);
    Py_XDECREF(list);
    Py_XDECREF(args);
    return NULL;
}