#ifndef ML_NODE_H
#define ML_NODE_H

#include "micro_liquid/common.h"
#include "micro_liquid/context.h"
#include "micro_liquid/expression.h"
#include "micro_liquid/object_list.h"

typedef enum
{
    NODE_OUPUT,
    NODE_IF_TAG,
    NODE_FOR_TAG,
    NODE_IF_BLOCK,
    NODE_FOR_BLOCK,
    NODE_ELIF_BLOCK,
    NODE_ELSE_BLOCK,
    NODE_TEXT
} ML_NodeKind;

typedef struct ML_Node
{
    ML_NodeKind kind;
    struct ML_Node **children;
    Py_ssize_t child_count;
    ML_Expr *expr;
    PyObject *str;
} ML_Node;

/**
 * Return a new node.
 *
 * The new node takes ownership of `children` and `expr`, both of which will be
 * freed by `ML_Node_destroy`.
 *
 * A reference to `str` will be stolen and its reference count decremented by
 * ML_Node_destroy.
 *
 * Pass `NULL` for `children`, `expr` and/or `str` if the node has no children,
 * expression or associated string.
 */
ML_Node *ML_Node_new(ML_NodeKind kind, ML_Node **children,
                     Py_ssize_t child_count, ML_Expr *expr, PyObject *str);

void ML_Node_destroy(ML_Node *self);

// TODO: can render fail?
bool ML_Node_render(ML_Node *self, ML_Context *ctx, ML_ObjList *buf);

#endif
