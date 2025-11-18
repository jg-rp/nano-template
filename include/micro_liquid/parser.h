#ifndef ML_PARSER_H
#define ML_PARSER_H

#include "micro_liquid/common.h"
#include "micro_liquid/expression.h"
#include "micro_liquid/lexer.h"
#include "micro_liquid/node.h"
#include "micro_liquid/token.h"

// TODO: doc comments

typedef struct ML_NodeList
{
    ML_Node **items;
    Py_ssize_t size;
    Py_ssize_t capacity;
} ML_NodeList;

typedef struct ML_Parser
{
    PyObject *str;
    Py_ssize_t length;
    ML_Token **tokens;
    Py_ssize_t token_count;
    Py_ssize_t pos;
    ML_TokenKind whitespace_carry;
} ML_Parser;

ML_Parser *ML_Parser_new(PyObject *str, ML_Token **tokens,
                         Py_ssize_t token_count);

void ML_Parser_dealloc(ML_Parser *self);
ML_NodeList *ML_Parser_parse(ML_Parser *self, ML_TokenMask end);

// Helpers for building arrays of nodes.
ML_NodeList *ML_NodeList_new(void);
void ML_NodeList_dealloc(ML_NodeList *self);
void ML_NodeList_disown(ML_NodeList *self);
Py_ssize_t ML_NodeList_grow(ML_NodeList *self);
Py_ssize_t ML_NodeList_append(ML_NodeList *self, ML_Node *node);

#endif