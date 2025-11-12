#ifndef ML_NODE_H
#define ML_NODE_H

#include "micro_liquid/common.h"
#include "micro_liquid/context.h"
#include "micro_liquid/expression.h"

typedef enum
{
    NODE_OUPUT,
    NODE_IF_TAG,
    NODE_FOR_TAG,
    NODE_BLOCK,
    NODE_TEXT
} ML_NodeKind;

typedef struct ML_Node
{
    ML_NodeKind kind;
    struct ML_Node **children;
    Py_ssize_t child_count;
    ML_Expression *expr;
    PyObject *name; // str | None
} ML_Node;

ML_Node *ML_Node_new(ML_NodeKind kind, ML_Node **children,
                     Py_ssize_t child_count, ML_Expression *expr,
                     PyObject *name);

void ML_Node_destroy(ML_Node *self);
bool ML_Node_render(ML_Node *self, ML_Context *ctx, PyObject *buf);

#endif
