#ifndef ML_PARSER_H
#define ML_PARSER_H

#include "micro_liquid/common.h"
#include "micro_liquid/expression.h"
#include "micro_liquid/lexer.h"
#include "micro_liquid/node.h"
#include "micro_liquid/token.h"

// TODO: doc comments

typedef struct ML_Parser
{
    PyObject *str;
    Py_ssize_t length; // XXX: why do we need this?
    ML_Token **tokens;
    Py_ssize_t token_count;
    Py_ssize_t pos;
    ML_TokenKind whitespace_carry;
} ML_Parser;

ML_Parser *ML_Parser_new(PyObject *str, ML_Token **tokens,
                         Py_ssize_t token_count);

void ML_Parser_dealloc(ML_Parser *self);
int ML_Parser_parse(ML_Parser *self, ML_Node *out_node, ML_TokenMask end);

#endif