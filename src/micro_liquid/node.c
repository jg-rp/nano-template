#include "micro_liquid/node.h"

/**
 * Render `node` to `buf` in the given render context `ctx`.
 * @param buf list[str]
 */
typedef bool (*RenderFn)(ML_Node *node, ML_Context *ctx, PyObject *buf);

static bool render_output(ML_Node *node, ML_Context *ctx, PyObject *buf);
static bool render_if_tag(ML_Node *node, ML_Context *ctx, PyObject *buf);
static bool render_for_tag(ML_Node *node, ML_Context *ctx, PyObject *buf);
static bool render_text(ML_Node *node, ML_Context *ctx, PyObject *buf);
static bool render_block(ML_Node *node, ML_Context *ctx, PyObject *buf);
static bool render_conditional_block(ML_Node *node, ML_Context *ctx,
                                     PyObject *buf);

static RenderFn render_table[] = {
    [NODE_OUPUT] = render_output,
    [NODE_IF_TAG] = render_if_tag,
    [NODE_FOR_TAG] = render_for_tag,
    [NODE_TEXT] = render_text,
};

ML_Node *ML_Node_new(ML_NodeKind kind, ML_Node **children,
                     Py_ssize_t child_count, ML_Expression *expr, PyObject *str)
{
    ML_Node *node = PyMem_Malloc(sizeof(ML_Node));
    if (!node)
        return NULL;

    if (str)
        Py_INCREF(str);

    node->kind = kind;
    node->children = children;
    node->child_count = child_count;
    node->expr = expr;
    node->str = str;
    return node;
}

void ML_Node_destroy(ML_Node *self)
{
    if (!self)
        return;

    // Recursively destroy nodes.
    if (self->children)
    {
        for (Py_ssize_t i = 0; i < self->child_count; i++)
        {
            if (self->children[i])
                ML_Node_destroy(self->children[i]);
        }
        PyMem_Free(self->children);
    }

    if (self->expr)
    {
        ML_Expression_destroy(self->expr);
    }

    Py_XDECREF(self->str);
    PyMem_Free(self);
}

bool ML_Node_render(ML_Node *self, ML_Context *ctx, PyObject *buf)
{
    if (!self)
        return false;

    RenderFn fn = render_table[self->kind];

    if (!fn)
        return false;
    return fn(self, ctx, buf);
}
