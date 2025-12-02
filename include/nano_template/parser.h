// SPDX-License-Identifier: MIT

#ifndef NT_PARSER_H
#define NT_PARSER_H

#include "nano_template/allocator.h"
#include "nano_template/common.h"
#include "nano_template/expression.h"
#include "nano_template/lexer.h"
#include "nano_template/node.h"
#include "nano_template/token.h"

// TODO: add recursion depth tracking/limit?

typedef struct NT_Parser
{
    NT_Mem *mem;   // Allocator for the AST.
    PyObject *str; // Input string.

    NT_Token *tokens; // Owned tokens being parsed.
    Py_ssize_t token_count;
    Py_ssize_t pos; // Current index into tokens.

    NT_TokenKind whitespace_carry; // Preceding whitespace control.
} NT_Parser;

/// @brief Allocate and initialize a new NT_Parser.
/// @return A pointer to the new parse, or NULL on failure with an exception
/// set.
NT_Parser *NT_Parser_new(NT_Mem *mem, PyObject *str, NT_Token *tokens,
                         Py_ssize_t token_count);

void NT_Parser_free(NT_Parser *p);

/// @brief Parser entry point.
/// @return A new node that is the root of the syntax tree, or NULL on failure
/// with an exception set.
NT_Node *NT_Parser_parse_root(NT_Parser *p);

#endif