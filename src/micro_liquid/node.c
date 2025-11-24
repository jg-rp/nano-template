#include "micro_liquid/node.h"
#include "micro_liquid/string_buffer.h"

/// @brief Render `node` to `buf` with data from render context `ctx`.
typedef int (*RenderFn)(ML_Node *node, ML_Context *ctx, PyObject *buf);

static int render_output(ML_Node *node, ML_Context *ctx, PyObject *buf);
static int render_if_tag(ML_Node *node, ML_Context *ctx, PyObject *buf);
static int render_for_tag(ML_Node *node, ML_Context *ctx, PyObject *buf);
static int render_text(ML_Node *node, ML_Context *ctx, PyObject *buf);

static RenderFn render_table[] = {
    [NODE_OUPUT] = render_output,
    [NODE_IF_TAG] = render_if_tag,
    [NODE_FOR_TAG] = render_for_tag,
    [NODE_TEXT] = render_text,
};

static int render_block(ML_Node *node, ML_Context *ctx, PyObject *buf);

/// @brief Render node->children if node->expr is truthy.
/// @return 1 if expr is truthy, 0 if expr is falsy, -1 on error.
static int render_conditional_block(ML_Node *node, ML_Context *ctx,
                                    PyObject *buf);

/// @brief Get an iterator for object `op`.
/// @return 0 on success, 1 if op is not iterable, -1 on error.
static int iter(PyObject *op, PyObject **out_iter);

ML_Node *ML_Node_new(ML_NodeKind kind)
{
    ML_Node *node = PyMem_Malloc(sizeof(ML_Node));

    if (!node)
    {
        PyErr_NoMemory();
        return NULL;
    }

    node->kind = kind;
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    node->expr = NULL;
    node->str = NULL;
    return node;
}

void ML_Node_dealloc(ML_Node *self)
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
                ML_Node_dealloc(self->children[i]);
            }
        }
        PyMem_Free(self->children);
        self->children = NULL;
        self->child_count = 0;
        self->child_capacity = 0;
    }

    if (self->expr)
    {
        ML_Expression_dealloc(self->expr);
        self->expr = NULL;
    }

    Py_XDECREF(self->str);
    self->str = NULL;
    PyMem_Free(self);
}

int ML_Node_render(ML_Node *self, ML_Context *ctx, PyObject *buf)
{
    if (!self)
    {
        return -1;
    }

    RenderFn fn = render_table[self->kind];

    if (!fn)
    {
        return -1;
    }
    return fn(self, ctx, buf);
}

int ML_Node_add_child(ML_Node *self, ML_Node *node)
{
    if (self->child_count == self->child_capacity)
    {
        Py_ssize_t new_cap =
            (self->child_capacity == 0) ? 2 : (self->child_capacity * 2);

        ML_Node **new_children = NULL;

        if (!self->children)
        {
            new_children = PyMem_Malloc(sizeof(ML_Node *) * new_cap);
        }
        else
        {
            new_children =
                PyMem_Realloc(self->children, sizeof(ML_Node *) * new_cap);
        }

        if (!new_children)
        {
            PyErr_NoMemory();
            return -1;
        }

        self->children = new_children;
        self->child_capacity = new_cap;
    }

    self->children[self->child_count++] = node;
    return 0;
}

static int render_output(ML_Node *node, ML_Context *ctx, PyObject *buf)
{
    PyObject *str = NULL;
    PyObject *op = ML_Expression_evaluate(node->expr, ctx);

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

static int render_if_tag(ML_Node *node, ML_Context *ctx, PyObject *buf)
{
    Py_ssize_t child_count = node->child_count;
    ML_Node *child = NULL;
    int rv = 0;

    for (Py_ssize_t i = 0; i < child_count; i++)
    {
        child = node->children[i];

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

    return 0;
}

static int render_for_tag(ML_Node *node, ML_Context *ctx, PyObject *buf)
{
    if (node->child_count < 1)
    {
        return 0; // XXX: silently ignore internal error
    }

    PyObject *key = node->str;
    ML_Node *block = node->children[0];
    PyObject *op = NULL;
    PyObject *it = NULL;
    PyObject *namespace = NULL;
    PyObject *item = NULL;

    op = ML_Expression_evaluate(node->expr, ctx);
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
        if (node->child_count == 2)
        {
            // else block
            render_block(node->children[1], ctx, buf);
        }

        return 0;
    }

    namespace = PyDict_New();
    if (!namespace)
    {
        goto fail;
    }

    Py_INCREF(namespace);
    if (ML_Context_push(ctx, namespace) < 0)
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
    ML_Context_pop(ctx);
    Py_DECREF(namespace);

    if (!rendered && node->child_count == 2)
    {
        if (render_block(node->children[1], ctx, buf) < 0)
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

static int render_text(ML_Node *node, ML_Context *ctx, PyObject *buf)
{
    (void)ctx;

    if (!node->str)
    {
        return 0; // XXX: silently ignoreing internal error
    }

    return StringBuffer_append(buf, node->str);
}

static int render_block(ML_Node *node, ML_Context *ctx, PyObject *buf)
{
    for (Py_ssize_t i = 0; i < node->child_count; i++)
    {
        if (ML_Node_render(node->children[i], ctx, buf) < 0)
        {
            return -1;
        }
    }

    return 0;
}

static int render_conditional_block(ML_Node *node, ML_Context *ctx,
                                    PyObject *buf)
{
    if (!node->expr)
    {
        return 0;
    }

    PyObject *op = ML_Expression_evaluate(node->expr, ctx);
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

    // TODO: avoid intermediate list of items.
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