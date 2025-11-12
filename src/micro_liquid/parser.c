#include "micro_liquid/parser.h"

ML_Node **NodeList_new();
int **NodeList_append(ML_Node **self, ML_Node *node);
int **NodeList_extend(ML_Node **self, ML_Node **nodes);

ML_Parser *ML_Parser_new(PyObject *str, ML_Token **tokens,
                         Py_ssize_t token_count)
{
    ML_Parser *parser = PyMem_Malloc(sizeof(ML_Parser));
    if (!parser)
        return NULL;

    Py_INCREF(str);
    parser->str = str;
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->pos = 0;
    parser->whitespace_carry = NULL;
    return parser;
}

void ML_Parser_destroy(ML_Parser *self)
{
    // Some expressions steal a token, so we don't want to free those here.
    // Whenever a token is stolen, it must be replaced with NULL in
    // `self->tokens`.

    ML_Token **tokens = self->tokens;
    Py_ssize_t token_count = self->token_count;

    if (tokens)
    {
        for (Py_ssize_t j = 0; j < token_count; j++)
        {
            if (tokens[j])
                PyMem_Free(tokens[j]);
        }
        PyMem_Free(tokens);
    }

    Py_XDECREF(self->str);
    Py_XDECREF(self->whitespace_carry);
    PyMem_Free(self);
}

ML_Node **ML_Parser_parse(ML_Parser *self)
{
    ML_Node **nodes = NodeList_new();
    if (!nodes)
        return NULL;

    for (;;)
    {
        // TODO: ML_Parser_next() that keeps returning EOF
        ML_Token *token = self->tokens[self->pos++];
        ML_TokenKind kind = token->kind;
        ML_TokenKind peeked = self->tokens[self->pos]->kind;

        switch (kind)
        {
        case TOK_OTHER:
            // TODO: handle
            NodeList_append(nodes,
                            ML_Node_new(NODE_TEXT, NULL, 0, NULL,
                                        ML_Token_text(token, self->str)));
            break;
        case TOK_OUT_START:
            if (peeked == TOK_WC_HYPHEN || peeked == TOK_WC_TILDE)
                self->pos++;
            NodeList_append(nodes, ML_Parser_parse_output(self));
            break;
        case TOK_TAG_START:
            if (peeked == TOK_WC_HYPHEN || peeked == TOK_WC_TILDE)
                self->pos++;
            NodeList_append(nodes, ML_Parser_parse_tag(self));
            break;
        case TOK_EOF:
            return nodes;
        default:
            // TODO: set err message - unexpected token kind
            return NULL;
        }
    }
}