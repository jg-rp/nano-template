#include "micro_liquid/context.h"
#include "micro_liquid/py_token_view.h"

Py_ssize_t ML_Context_push(ML_Context *self, PyObject *namespace)
{
    if (self->size >= self->capacity)
    {
        size_t new_cap = (self->capacity == 0) ? 4 : (self->capacity * 2);
        PyObject **new_items =
            PyMem_Realloc(self->scope, sizeof(PyObject *) * new_cap);
        if (!new_items)
        {
            PyErr_NoMemory();
            return -1;
        }

        self->scope = new_items;
        self->capacity = new_cap;
    }

    Py_INCREF(namespace);
    self->scope[self->size++] = namespace;
    return 0;
}

ML_Context *ML_Context_new(PyObject *str, PyObject *globals,
                           PyObject *serializer, PyObject *undefined)
{
    ML_Context *ctx = PyMem_Malloc(sizeof(ML_Context));
    if (!ctx)
    {
        PyErr_NoMemory();
        return NULL;
    }

    // TODO: defensively ensure `globals` is a mapping etc.?

    Py_INCREF(str);
    Py_INCREF(serializer);
    Py_INCREF(undefined);

    ctx->scope = NULL;
    ctx->size = 0;
    ctx->capacity = 0;
    ctx->serializer = serializer;
    ctx->undefined = undefined;

    if (globals)
    {
        Py_INCREF(globals);
        ML_Context_push(ctx, globals);
    }

    return ctx;
}

void ML_Context_destroy(ML_Context *self)
{
    Py_XDECREF(self->str);

    for (Py_ssize_t i = 0; i < self->size; i++)
    {
        Py_XDECREF(self->scope[i]);
    }

    PyMem_Free(self->scope);
    Py_XDECREF(self->serializer);
    Py_XDECREF(self->undefined);
    PyMem_Free(self);
}

PyObject *ML_Context_get(ML_Context *self, PyObject *key, ML_Token *token)
{
    PyObject *obj = NULL;
    PyObject *args = NULL;
    PyObject *token_view = NULL;

    for (Py_ssize_t i = self->size - 1; i >= 0; i--)
    {
        obj = PyObject_GetItem(self->scope[i], key);
        if (obj)
        {
            Py_INCREF(obj);
            return obj;
        }
        else
        {
            PyErr_Clear();
        }
    }

    token_view =
        MLPY_TokenView_new(self->str, token->start, token->end, token->kind);

    if (!token_view)
        goto fail;

    args = Py_BuildValue("(O, O)", key, token_view);
    PyObject *undef = PyObject_CallObject(self->undefined, args);
    if (!undef)
        goto fail;

    return undef;

fail:
    Py_XDECREF(obj);
    Py_XDECREF(token_view);
    Py_XDECREF(args);
    return NULL;
}

PyObject *ML_Context_pop(ML_Context *self)
{
    return self->size ? self->scope[--self->size] : NULL;
}
