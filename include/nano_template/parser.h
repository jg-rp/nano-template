#ifndef NT_PARSER_H
#define NT_PARSER_H

#include "nano_template/allocator.h"
#include "nano_template/common.h"
#include "nano_template/expression.h"
#include "nano_template/lexer.h"
#include "nano_template/node.h"
#include "nano_template/token.h"

// TODO: doc comments
// TODO: add recursion depth tracking

typedef struct NT_Parser
{
    NT_Mem *mem;
    PyObject *str;
    NT_Token *tokens;
    Py_ssize_t token_count;
    Py_ssize_t pos;
    NT_TokenKind whitespace_carry;
} NT_Parser;

NT_Parser *NT_Parser_new(NT_Mem *mem, PyObject *str, NT_Token *tokens,
                         Py_ssize_t token_count);

void NT_Parser_free(NT_Parser *p);

NT_Node *NT_Parser_parse_root(NT_Parser *p);

int NT_Parser_parse(NT_Parser *p, NT_Node *out_node, NT_TokenMask end);

#endif