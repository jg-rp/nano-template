// SPDX-License-Identifier: MIT

#ifndef NT_NODE_H
#define NT_NODE_H

#include "nano_template/common.h"
#include "nano_template/context.h"
#include "nano_template/expression.h"

#define NT_CHILDREN_PER_PAGE 4

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
} NT_NodeKind;

/// @brief One block of a paged array (unrolled linked list) holding AST nodes.
typedef struct NT_NodePage
{
    struct NT_NodePage *next;
    Py_ssize_t count;
    struct NT_Node *nodes[NT_CHILDREN_PER_PAGE];
} NT_NodePage;

typedef struct NT_Node
{
    // Paged array holding child nodes.
    NT_NodePage *head;
    NT_NodePage *tail;

    // Optional expression, like a conditional or loop expression.
    NT_Expr *expr;

    // Optional string object associated with the node. Like the name of a loop
    // variable or literal text.
    PyObject *str;

    NT_NodeKind kind;
} NT_Node;

/// @brief Render node `node` to `buf` with data from `ctx`.
/// @return 0 on success, -1 on failure with a Python error set.
int NT_Node_render(const NT_Node *node, NT_RenderContext *ctx, PyObject *buf);

#endif
