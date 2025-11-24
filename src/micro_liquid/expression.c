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

ML_Expr *ML_Expression_new(ML_ExpressionKind kind, ML_Token *token)
{
    ML_Expr *expr = PyMem_Malloc(sizeof(ML_Expr));
    if (!expr)
    {
        PyErr_NoMemory();
        return NULL;
    }

    expr->kind = kind;
    expr->token = token;

    expr->child_count = 0;
    expr->child_capacity = 0;
    expr->children = NULL;

    expr->object_count = 0;
    expr->object_capacity = 0;
    expr->objects = NULL;
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
        self->children = NULL;
        self->child_count = 0;
        self->child_capacity = 0;
    }

    if (self->token)
    {
        PyMem_Free(self->token);
        self->token = NULL;
    }

    if (self->objects)
    {
        for (Py_ssize_t i = 0; i < self->object_count; i++)
        {
            Py_XDECREF(self->objects[i]);
        }
        PyMem_Free(self->objects);
        self->objects = NULL;
    }

    PyMem_Free(self);
}

int ML_Expression_add_child(ML_Expr *self, ML_Expr *expr)
{
    if (self->child_count == self->child_capacity)
    {
        Py_ssize_t new_cap =
            (self->child_capacity == 0) ? 2 : (self->child_capacity * 2);

        ML_Expr **new_children = NULL;

        if (!self->children)
        {
            new_children = PyMem_Malloc(sizeof(ML_Expr *) * new_cap);
        }
        else
        {
            new_children =
                PyMem_Realloc(self->children, sizeof(ML_Expr *) * new_cap);
        }

        if (!new_children)
        {
            PyErr_NoMemory();
            return -1;
        }

        self->children = new_children;
        self->child_capacity = new_cap;
    }

    self->children[self->child_count++] = expr;
    return 0;
}

int ML_Expression_add_obj(ML_Expr *self, PyObject *obj)
{
    if (self->object_count == self->object_capacity)
    {
        Py_ssize_t new_cap =
            (self->object_capacity == 0) ? 4 : (self->object_capacity * 2);

        PyObject **new_objects = NULL;

        if (!self->objects)
        {
            new_objects = PyMem_Malloc(sizeof(PyObject *) * new_cap);
        }
        else
        {
            new_objects =
                PyMem_Realloc(self->objects, sizeof(PyObject *) * new_cap);
        }

        if (!new_objects)
        {
            PyErr_NoMemory();
            return -1;
        }

        self->objects = new_objects;
        self->object_capacity = new_cap;
    }

    Py_INCREF(obj);
    self->objects[self->object_count++] = obj;
    return 0;
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
    return Py_NewRef(expr->objects[0]);
}

static PyObject *eval_var_expr(ML_Expr *expr, ML_Context *ctx)
{
    Py_ssize_t count = expr->object_count;
    PyObject **segments = expr->objects;
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