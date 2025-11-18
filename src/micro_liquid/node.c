#include "micro_liquid/node.h"

/// @brief Render `node` to `buf` in the given render context `ctx`.
typedef Py_ssize_t (*RenderFn)(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);

static Py_ssize_t render_output(ML_Node *node, ML_Context *ctx,
                                ML_ObjList *buf);
static Py_ssize_t render_if_tag(ML_Node *node, ML_Context *ctx,
                                ML_ObjList *buf);
static Py_ssize_t render_for_tag(ML_Node *node, ML_Context *ctx,
                                 ML_ObjList *buf);
static Py_ssize_t render_text(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
static Py_ssize_t render_block(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
static Py_ssize_t render_conditional_block(ML_Node *node, ML_Context *ctx,
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

Py_ssize_t ML_Node_render(ML_Node *self, ML_Context *ctx, ML_ObjList *buf)
{
    if (!self)
        return -1;

    RenderFn fn = render_table[self->kind];

    if (!fn)
        return -1;
    return fn(self, ctx, buf);
}

static Py_ssize_t render_output(ML_Node *node, ML_Context *ctx, ML_ObjList *buf)
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

static Py_ssize_t render_if_tag(ML_Node *node, ML_Context *ctx, ML_ObjList *buf)
{
    PY_TODO_I();
}

static Py_ssize_t render_for_tag(ML_Node *node, ML_Context *ctx,
                                 ML_ObjList *buf)
{
    PY_TODO_I();
}

static Py_ssize_t render_text(ML_Node *node, ML_Context *ctx, ML_ObjList *buf)
{
    if (!node->str)
        return 0; // XXX: silently ignoreing

    // XXX: assumes str is a string
    return ML_ObjList_append(buf, node->str);
}

static Py_ssize_t render_block(ML_Node *node, ML_Context *ctx, ML_ObjList *buf)
{
    PY_TODO_I();
}

static Py_ssize_t render_conditional_block(ML_Node *node, ML_Context *ctx,
                                           ML_ObjList *buf)
{
    PY_TODO_I();
}