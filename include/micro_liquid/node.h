#ifndef ML_NODE_H
#define ML_NODE_H

#include "micro_liquid/common.h"
#include "micro_liquid/context.h"
#include "micro_liquid/expression.h"

#define ML_CHILDREN_PER_PAGE 4

/// @brief AST node kinds.
typedef enum
{
    NODE_ROOT = 1,
    NODE_OUPUT,
    NODE_IF_TAG,
    NODE_FOR_TAG,
    NODE_IF_BLOCK,
    NODE_FOR_BLOCK,
    NODE_ELIF_BLOCK,
    NODE_ELSE_BLOCK,
    NODE_TEXT
} ML_NodeKind;

/// @brief One block of a paged array holding AST nodes.
typedef struct ML_NodePage
{
    struct ML_NodePage *next;
    Py_ssize_t count;
    struct ML_Node *nodes[ML_CHILDREN_PER_PAGE];
} ML_NodePage;

/// @brief Internal AST node.
typedef struct ML_Node
{
    // Paged array holding child nodes.
    ML_NodePage *head;
    ML_NodePage *tail;

    // Optional expression, like a conditional or loop expression.
    ML_Expr *expr;

    // Optional string object associated with the node. Like the name of a loop
    // variable or literal text.
    PyObject *str;

    ML_NodeKind kind;
} ML_Node;

/// @brief Render node `node` to `buf` with data from `ctx`.
/// @return 0 on success, -1 on failure with a Python error set.
int ML_Node_render(ML_Node *node, ML_Context *ctx, PyObject *buf);

#endif
