// SPDX-License-Identifier: MIT

#include "nano_template/node.h"
#include "nano_template/string_buffer.h"

/// @brief Render `node` to `buf` with data from render context `ctx`.
typedef int (*RenderFn)(const NT_Node *node, NT_RenderContext *ctx,
                        PyObject *buf);

static int render_output(const NT_Node *node, NT_RenderContext *ctx,
                         PyObject *buf);

static int render_if_tag(const NT_Node *node, NT_RenderContext *ctx,
                         PyObject *buf);

static int render_for_tag(const NT_Node *node, NT_RenderContext *ctx,
                          PyObject *buf);

static int render_text(const NT_Node *node, NT_RenderContext *ctx,
                       PyObject *buf);

static RenderFn render_table[] = {
    [NODE_OUPUT] = render_output,
    [NODE_IF_TAG] = render_if_tag,
    [NODE_FOR_TAG] = render_for_tag,
    [NODE_TEXT] = render_text,
};

static int render_block(NT_Node *node, NT_RenderContext *ctx, PyObject *buf);

/// @brief Render node->children if node->expr is truthy.
/// @return 1 if expr is truthy, 0 if expr is falsy, -1 on error.
static int render_conditional_block(NT_Node *node, NT_RenderContext *ctx,
                                    PyObject *buf);

/// @brief Get an iterator for object `op`.
/// @return 0 on success, 1 if op is not iterable, -1 on error.
static int iter(PyObject *op, PyObject **out_iter);

int NT_Node_render(const NT_Node *node, NT_RenderContext *ctx, PyObject *buf)
{
    if (!node)
    {
        return -1;
    }

    RenderFn fn = render_table[node->kind];

    if (!fn)
    {
        return -1;
    }
    return fn(node, ctx, buf);
}

static int render_output(const NT_Node *node, NT_RenderContext *ctx,
                         PyObject *buf)
{
    PyObject *str = NULL;
    PyObject *op = NT_Expr_evaluate(node->expr, ctx);

    if (!op)
    {
        return -1;
    }

    str = PyObject_CallFunctionObjArgs(ctx->serializer, op, NULL);

    if (!str)
    {
        goto fail;
    }

    int rv = StringBuffer_append(buf, str);
    if (rv < 0)
    {
        goto fail;
    }

    Py_DECREF(str);
    Py_DECREF(op);
    return rv;

fail:
    Py_XDECREF(op);
    Py_XDECREF(str);
    return -1;
}

static int render_if_tag(const NT_Node *node, NT_RenderContext *ctx,
                         PyObject *buf)
{
    int rv = 0;
    NT_Node *child = NULL;
    NT_NodePage *page = node->head;

    while (page)
    {
        for (Py_ssize_t i = 0; i < page->count; i++)
        {
            child = page->nodes[i];

            if (child->kind == NODE_ELSE_BLOCK)
            {
                return render_block(child, ctx, buf);
            }

            rv = render_conditional_block(child, ctx, buf);

            if (rv == 0)
            {
                continue;
            }

            return rv;
        }

        page = page->next;
    }

    return 0;
}

static int render_for_tag(const NT_Node *node, NT_RenderContext *ctx,
                          PyObject *buf)
{
    if (!node->head)
    {
        return 0;
    }

    // We assume a single page. A for tag can have 1 or 2 children.
    Py_ssize_t child_count = node->head->count;

    if (child_count < 1)
    {
        return 0;
    }

    PyObject *key = node->str;
    NT_Node *block = node->head->nodes[0];
    PyObject *op = NULL;
    PyObject *it = NULL;
    PyObject *namespace = NULL;
    PyObject *item = NULL;

    op = NT_Expr_evaluate(node->expr, ctx);
    if (!op)
    {
        return -1;
    }

    int rc = iter(op, &it);
    Py_DECREF(op);

    if (rc == -1)
    {
        return -1;
    }

    if (rc == 1)
    {
        // not iterable
        if (child_count == 2)
        {
            // else block
            render_block(node->head->nodes[1], ctx, buf);
        }

        return 0;
    }

    namespace = PyDict_New();
    if (!namespace)
    {
        goto fail;
    }

    if (NT_RenderContext_push(ctx, namespace) < 0)
    {
        goto fail;
    }

    bool rendered = false;

    for (;;)
    {
        item = PyIter_Next(it);
        if (!item)
        {
            if (PyErr_Occurred())
            {
                goto fail;
            }
            break;
        }

        if (PyDict_SetItem(namespace, key, item) < 0)
        {
            goto fail;
        }

        Py_DECREF(item);
        rendered = true;

        if (render_block(block, ctx, buf) < 0)
        {
            goto fail;
        }
    }

    Py_DECREF(it);
    NT_RenderContext_pop(ctx);
    Py_DECREF(namespace);

    if (!rendered && child_count == 2)
    {
        if (render_block(node->head->nodes[1], ctx, buf) < 0)
        {
            goto fail;
        }
    }

    return 0;

fail:
    Py_XDECREF(namespace);
    Py_XDECREF(it);
    Py_XDECREF(item);
    return -1;
}

static int render_text(const NT_Node *node, NT_RenderContext *ctx,
                       PyObject *buf)
{
    (void)ctx;

    if (!node->str)
    {
        return 0;
    }

    return StringBuffer_append(buf, node->str);
}

static int render_block(NT_Node *node, NT_RenderContext *ctx, PyObject *buf)
{
    NT_NodePage *page = node->head;
    while (page)
    {
        for (Py_ssize_t i = 0; i < page->count; i++)
        {
            if (NT_Node_render(page->nodes[i], ctx, buf) < 0)
            {
                return -1;
            }
        }
        page = page->next;
    }

    return 0;
}

static int render_conditional_block(NT_Node *node, NT_RenderContext *ctx,
                                    PyObject *buf)
{
    if (!node->expr)
    {
        return 0;
    }

    PyObject *op = NT_Expr_evaluate(node->expr, ctx);
    if (!op)
    {
        return -1;
    }

    int truthy = PyObject_IsTrue(op);
    Py_XDECREF(op);

    if (!truthy)
    {
        return 0;
    }

    if (render_block(node, ctx, buf) < 0)
    {
        return -1;
    }

    return 1;
}

static int iter(PyObject *op, PyObject **out_iter)
{
    PyObject *it = NULL;
    *out_iter = NULL;

    PyObject *items = PyMapping_Items(op);

    if (items)
    {
        it = PyObject_GetIter(items);
        Py_DECREF(items);

        if (!it)
        {
            return -1;
        }

        *out_iter = it;
        return 0;
    }

    if (PyErr_ExceptionMatches(PyExc_TypeError) ||
        PyErr_ExceptionMatches(PyExc_AttributeError))
    {
        PyErr_Clear(); // not a mapping
    }
    else if (PyErr_Occurred())
    {
        return -1; // unexpected error
    }

    it = PyObject_GetIter(op);
    if (it)
    {
        *out_iter = it;
        return 0;
    }

    if (PyErr_ExceptionMatches(PyExc_TypeError))
    {
        PyErr_Clear(); // not iterable
        return 1;
    }

    return -1; // unexpected error
}