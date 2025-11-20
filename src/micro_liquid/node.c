#include "micro_liquid/node.h"

/// @brief Render `node` to `buf` with data from render context `ctx`.
typedef int (*RenderFn)(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);

static int render_output(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
static int render_if_tag(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
static int render_for_tag(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
static int render_text(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
static int render_block(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);

/// @brief Render node->children if node->expr is truthy.
/// @return 1 if expr is truthy, 0 if expr is falsy, -1 on error.
static int render_conditional_block(ML_Node *node, ML_Context *ctx,
                                    ML_ObjList *buf);

static RenderFn render_table[] = {
    [NODE_OUPUT] = render_output,
    [NODE_IF_TAG] = render_if_tag,
    [NODE_FOR_TAG] = render_for_tag,
    [NODE_TEXT] = render_text,
};

ML_Node *ML_Node_new(ML_NodeKind kind, ML_Node **children,
                     Py_ssize_t child_count, ML_Expr *expr, PyObject *str)
{
    ML_Node *node = PyMem_Malloc(sizeof(ML_Node));

    if (!node)
    {
        PyErr_NoMemory();
        return NULL;
    }

    node->kind = kind;
    node->children = children;
    node->child_count = child_count;
    node->expr = expr;
    node->str = str;
    return node;
}

void ML_Node_dealloc(ML_Node *self)
{
    if (!self)
        return;

    // Recursively destroy nodes.
    if (self->children)
    {
        for (Py_ssize_t i = 0; i < self->child_count; i++)
        {
            if (self->children[i])
                ML_Node_dealloc(self->children[i]);
        }
        PyMem_Free(self->children);
    }

    if (self->expr)
    {
        ML_Expression_dealloc(self->expr);
    }

    Py_XDECREF(self->str);
    PyMem_Free(self);
}

int ML_Node_render(ML_Node *self, ML_Context *ctx, ML_ObjList *buf)
{
    if (!self)
        return -1;

    RenderFn fn = render_table[self->kind];

    if (!fn)
        return -1;
    return fn(self, ctx, buf);
}

static int render_output(ML_Node *node, ML_Context *ctx, ML_ObjList *buf)
{
    PyObject *str = NULL;
    PyObject *op = ML_Expression_evaluate(node->expr, ctx);

    if (!op)
        return -1;

    str = PyObject_CallFunctionObjArgs(ctx->serializer, op, NULL);

    if (!str)
        goto fail;

    Py_ssize_t rv = ML_ObjList_append(buf, str);
    if (rv < 0)
        goto fail;

    Py_DECREF(op);
    return rv;

fail:
    Py_XDECREF(op);
    Py_XDECREF(str);
    return -1;
}

static int render_if_tag(ML_Node *node, ML_Context *ctx, ML_ObjList *buf)
{
    Py_ssize_t child_count = node->child_count;
    ML_Node *child = NULL;
    int rv = 0;

    for (Py_ssize_t i = 0; i < child_count; i++)
    {
        child = node->children[i];

        if (child->kind == NODE_ELSE_BLOCK)
            return render_block(child, ctx, buf);

        rv = render_conditional_block(child, ctx, buf);

        if (rv == 0)
            continue;

        return rv;
    }

    return 0;
}

static int render_for_tag(ML_Node *node, ML_Context *ctx, ML_ObjList *buf)
{
    PY_TODO_I();
}

static int render_text(ML_Node *node, ML_Context *ctx, ML_ObjList *buf)
{
    (void)ctx;

    if (!node->str)
        return 0; // XXX: silently ignoreing internal error

    return ML_ObjList_append(buf, node->str);
}

static int render_block(ML_Node *node, ML_Context *ctx, ML_ObjList *buf)
{
    for (Py_ssize_t i = 0; i < node->child_count; i++)
    {
        if (ML_Node_render(node->children[i], ctx, buf) < 0)
            return -1;
    }

    return 0;
}

static int render_conditional_block(ML_Node *node, ML_Context *ctx,
                                    ML_ObjList *buf)
{
    if (!node->expr)
        return 0;

    PyObject *op = ML_Expression_evaluate(node->expr, ctx);
    if (!op)
        return -1;

    int truthy = PyObject_IsTrue(op);
    Py_XDECREF(op);

    if (!truthy)
        return 0;

    if (render_block(node, ctx, buf) < 0)
        return -1;

    return 1;
}