#ifndef ML_NODE_H
#define ML_NODE_H

#include "micro_liquid/common.h"
#include "micro_liquid/context.h"
#include "micro_liquid/expression.h"
#include "micro_liquid/object_list.h"

/// @brief AST node kinds.
typedef enum
{
    NODE_OUPUT = 1,
    NODE_IF_TAG,
    NODE_FOR_TAG,
    NODE_IF_BLOCK,
    NODE_FOR_BLOCK,
    NODE_ELIF_BLOCK,
    NODE_ELSE_BLOCK,
    NODE_TEXT
} ML_NodeKind;

/// @brief Internal AST node.
typedef struct ML_Node
{
    ML_NodeKind kind;
    struct ML_Node **children;
    Py_ssize_t child_count;
    ML_Expr *expr;
    PyObject *str;
} ML_Node;

/// @brief Allocate and initialize a new ML_Node.
/// Take ownership of `children` and `expr` and steal a reference to `str`.
/// `children`, `expr` and/or `str` an be NULL if the node has no children,
/// expression or associated string.
/// @return Newly allocated ML_Node*, or NULL on memory error.
ML_Node *ML_Node_new(ML_NodeKind kind, ML_Node **children,
                     Py_ssize_t child_count, ML_Expr *expr, PyObject *str);

void ML_Node_dealloc(ML_Node *self);

/// @brief Render a node to `buf` with data from `ctx`.
/// @return 0 on success, -1 on failure with a Python error set.
int ML_Node_render(ML_Node *self, ML_Context *ctx, ML_ObjList *buf);

#endif
