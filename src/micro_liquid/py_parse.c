#include "micro_liquid/py_parse.h"
#include "micro_liquid/lexer.h"
#include "micro_liquid/parser.h"
#include "micro_liquid/py_template.h"

PyObject *parse(PyObject *Py_UNUSED(self), PyObject *src)
{
    ML_Token **tokens = NULL;
    Py_ssize_t token_count = 0;
    ML_Lexer *lexer = NULL;
    ML_Parser *parser = NULL;
    ML_NodeList *nodes = NULL;
    PyObject *template = NULL;

    lexer = ML_Lexer_new(src);
    if (!lexer)
        return NULL;

    tokens = ML_Lexer_scan(lexer, &token_count);
    if (!tokens)
        goto fail;

    parser = ML_Parser_new(src, tokens, token_count);
    if (!parser)
        goto fail;

    nodes = ML_Parser_parse(parser, 0);
    if (!nodes)
        goto fail;

    template = MLPY_Template_new(src, nodes->items, nodes->size);
    if (!template)
        goto fail;

    ML_Lexer_dealloc(lexer);
    ML_Parser_dealloc(parser);
    ML_NodeList_disown(nodes);
    return template;

fail:
    if (!parser && tokens)
    {
        for (Py_ssize_t j = 0; j < token_count; j++)
        {
            if (tokens[j])
                PyMem_Free(tokens[j]);
        }
        PyMem_Free(tokens);
    }

    if (lexer)
        ML_Lexer_dealloc(lexer);

    if (parser)
        ML_Parser_dealloc(parser);

    if (!template && nodes)
    {
        ML_NodeList_dealloc(nodes);
    }

    return NULL;
}
