#include "micro_liquid/context.h"

int ML_Context_push(ML_Context *self, PyObject *namespace)
{
    if (self->size >= self->capacity)
    {
        Py_ssize_t new_cap = (self->capacity == 0) ? 4 : (self->capacity * 2);
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

    Py_INCREF(str);
    Py_INCREF(serializer);
    Py_INCREF(undefined);

    ctx->scope = NULL;
    ctx->size = 0;
    ctx->capacity = 0;
    ctx->serializer = serializer;
    ctx->undefined = undefined;
    ML_Context_push(ctx, globals);

    return ctx;
}

void ML_Context_dealloc(ML_Context *self)
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

PyObject *ML_Context_get(ML_Context *self, PyObject *key, void *undefined)
{
    PyObject *obj = NULL;

    for (Py_ssize_t i = self->size - 1; i >= 0; i--)
    {
        obj = PyObject_GetItem(self->scope[i], key);
        if (obj)
        {
            return obj;
        }
        else
            PyErr_Clear();
    }

    return undefined; // TODO: fix me
}

void ML_Context_pop(ML_Context *self)
{
    if (self->size > 0)
        Py_DECREF(self->scope[--self->size]);
}
