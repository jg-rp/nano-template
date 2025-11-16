#include "micro_liquid/expression.h"

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

void ML_Expression_destroy(ML_Expr *self)
{
    if (!self)
        return;

    if (self->children)
    {
        for (Py_ssize_t i = 0; i < self->child_count; i++)
        {
            if (self->children[i])
                ML_Expression_destroy(self->children[i]);
        }
        PyMem_Free(self->children);
    }

    if (self->token)
        PyMem_Free(self->token);

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

// TODO: PyObject *ML_Expression_evaluate(ML_Expr *self, ML_Context *ctx);