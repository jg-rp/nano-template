#include "micro_liquid/node.h"

/// @brief Render `node` to `buf` in the given render context `ctx`.
typedef int (*RenderFn)(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);

static int render_output(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
static int render_if_tag(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
static int render_for_tag(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
static int render_text(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
static int render_block(ML_Node *node, ML_Context *ctx, ML_ObjList *buf);
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
        ML_Expression_destroy(self->expr);
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
