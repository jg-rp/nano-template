#include "micro_liquid/node.h"
#include "micro_liquid/context.h"

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

bool ML_Node_render(ML_Node *self, ML_Context *ctx, PyObject *buf)
{
    if (!self)
        return false;

    RenderFn fn = render_table[self->kind];

    if (!fn)
        return false;
    return fn(self, ctx, buf);
}
