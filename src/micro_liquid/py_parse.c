#include "micro_liquid/py_parse.h"
#include "micro_liquid/lexer.h"
#include "micro_liquid/parser.h"
#include "micro_liquid/py_template.h"

PyObject *parse(PyObject *Py_UNUSED(self), PyObject *src)
{
    ML_Lexer *lexer = NULL;
    ML_Node *root = NULL;
    ML_Parser *parser = NULL;
    ML_Token *tokens = NULL;
    Py_ssize_t token_count = 0;
    PyObject *template = NULL;

    lexer = ML_Lexer_new(src);
    if (!lexer)
    {
        return NULL;
    }

    tokens = ML_Lexer_scan(lexer, &token_count);
    if (!tokens)
    {
        goto fail;
    }

    // TODO: have Parser_new create the lexer and tokens so we don't
    // need to worry about who owns them.

    parser = ML_Parser_new(src, tokens, token_count);
    if (!parser)
    {
        goto fail;
    }

    tokens = NULL;

    root = ML_Node_new(NODE_ROOT);
    if (!root)
    {
        goto fail;
    }

    if (ML_Parser_parse(parser, root, 0) < 0)
    {
        goto fail;
    }

    template = MLPY_Template_new(src, root);
    if (!template)
    {
        goto fail;
    }

    root = NULL;
    ML_Lexer_dealloc(lexer);
    ML_Parser_dealloc(parser);
    return template;

fail:
    if (tokens)
    {
        PyMem_Free(tokens);
        tokens = NULL;
    }

    if (lexer)
    {
        ML_Lexer_dealloc(lexer);
        lexer = NULL;
    }

    if (parser)
    {
        ML_Parser_dealloc(parser);
        parser = NULL;
    }

    if (root)
    {
        ML_Node_dealloc(root);
        root = NULL;
    }

    return NULL;
}
