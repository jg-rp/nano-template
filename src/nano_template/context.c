#include "nano_template/context.h"

NT_Context *NT_Context_new(PyObject *str, PyObject *globals,
                           PyObject *serializer, PyObject *undefined)
{
    NT_Context *ctx = PyMem_Malloc(sizeof(NT_Context));
    if (!ctx)
    {
        PyErr_NoMemory();
        return NULL;
    }

    Py_INCREF(str);
    Py_INCREF(serializer);
    Py_INCREF(undefined);

    ctx->str = str;
    ctx->scope = NULL;
    ctx->size = 0;
    ctx->capacity = 0;
    ctx->serializer = serializer;
    ctx->undefined = undefined;
    NT_Context_push(ctx, globals);

    return ctx;
}

void NT_Context_free(NT_Context *self)
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

int NT_Context_get(NT_Context *self, PyObject *key, PyObject **out)
{
    PyObject *obj = NULL;

    for (Py_ssize_t i = self->size - 1; i >= 0; i--)
    {
        obj = PyObject_GetItem(self->scope[i], key);
        if (obj)
        {
            *out = obj;
            return 0;
        }
        PyErr_Clear();
    }

    return -1;
}

int NT_Context_push(NT_Context *self, PyObject *namespace)
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

void NT_Context_pop(NT_Context *self)
{
    if (self->size > 0)
    {
        Py_DECREF(self->scope[--self->size]);
    }
}
