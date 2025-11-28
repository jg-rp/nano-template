#include "micro_liquid/py_parse.h"
#include "micro_liquid/allocator.h"
#include "micro_liquid/lexer.h"
#include "micro_liquid/parser.h"
#include "micro_liquid/py_template.h"

PyObject *parse(PyObject *Py_UNUSED(self), PyObject *src)
{
    ML_Lexer *lexer = NULL;
    ML_Mem *ast = NULL;
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

    ast = ML_Mem_new();
    if (!ast)
    {
        goto fail;
    }

    parser = ML_Parser_new(ast, src, tokens, token_count);
    if (!parser)
    {
        goto fail;
    }

    tokens = NULL;

    root = ML_Parser_parse_root(parser);
    if (!root)
    {
        goto fail;
    }

    template = MLPY_Template_new(src, root, ast);
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

    if (ast)
    {
        ML_Mem_free(ast);
        ast = NULL;
    }

    return NULL;
}
