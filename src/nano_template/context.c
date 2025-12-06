// SPDX-License-Identifier: MIT

#include "nano_template/context.h"

NT_RenderContext *NT_RenderContext_new(PyObject *str, PyObject *globals,
                                       PyObject *serializer,
                                       PyObject *undefined)
{
    NT_RenderContext *ctx = PyMem_Malloc(sizeof(NT_RenderContext));
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

    if (NT_RenderContext_push(ctx, globals) < 0)
    {
        NT_RenderContext_free(ctx);
        ctx = NULL;
        return NULL;
    }

    return ctx;
}

void NT_RenderContext_free(NT_RenderContext *ctx)
{
    Py_XDECREF(ctx->str);

    for (Py_ssize_t i = 0; i < ctx->size; i++)
    {
        Py_XDECREF(ctx->scope[i]);
    }

    PyMem_Free(ctx->scope);
    Py_XDECREF(ctx->serializer);
    Py_XDECREF(ctx->undefined);
    PyMem_Free(ctx);
}

int NT_RenderContext_get(const NT_RenderContext *ctx, PyObject *key,
                         PyObject **out)
{
    PyObject *obj = NULL;

    for (Py_ssize_t i = ctx->size - 1; i >= 0; i--)
    {
        obj = PyObject_GetItem(ctx->scope[i], key);
        if (obj)
        {
            *out = obj;
            return 0;
        }
        PyErr_Clear();
    }

    return -1;
}

int NT_RenderContext_push(NT_RenderContext *ctx, PyObject *namespace)
{
    if (ctx->size >= ctx->capacity)
    {
        Py_ssize_t new_cap = (ctx->capacity == 0) ? 4 : (ctx->capacity * 2);
        PyObject **new_items =
            PyMem_Realloc(ctx->scope, sizeof(PyObject *) * new_cap);
        if (!new_items)
        {
            PyErr_NoMemory();
            return -1;
        }

        ctx->scope = new_items;
        ctx->capacity = new_cap;
    }

    Py_INCREF(namespace);
    ctx->scope[ctx->size++] = namespace;
    return 0;
}

void NT_RenderContext_pop(NT_RenderContext *ctx)
{
    if (ctx->size > 0)
    {
        Py_DECREF(ctx->scope[--ctx->size]);
    }
}
