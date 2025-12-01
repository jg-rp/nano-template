#include "nano_template/py_parse.h"
#include "nano_template/allocator.h"
#include "nano_template/lexer.h"
#include "nano_template/parser.h"
#include "nano_template/py_template.h"

PyObject *parse(PyObject *Py_UNUSED(self), PyObject *args)
{
    NT_Lexer *lexer = NULL;
    NT_Mem *ast = NULL;
    NT_Node *root = NULL;
    NT_Parser *parser = NULL;
    NT_Token *tokens = NULL;
    Py_ssize_t token_count = 0;
    PyObject *template = NULL;

    PyObject *src;
    PyObject *serializer;
    PyObject *undefined;

    if (!PyArg_ParseTuple(args, "OOO", &src, &serializer, &undefined))
    {
        return NULL;
    }

    // TODO: validate arguments

    lexer = NT_Lexer_new(src);
    if (!lexer)
    {
        return NULL;
    }

    tokens = NT_Lexer_scan(lexer, &token_count);
    if (!tokens)
    {
        goto fail;
    }

    ast = NT_Mem_new();
    if (!ast)
    {
        goto fail;
    }

    parser = NT_Parser_new(ast, src, tokens, token_count);
    if (!parser)
    {
        goto fail;
    }

    tokens = NULL;

    root = NT_Parser_parse_root(parser);
    if (!root)
    {
        goto fail;
    }

    template = NTPY_Template_new(src, root, ast, serializer, undefined);
    if (!template)
    {
        goto fail;
    }

    root = NULL;
    NT_Lexer_free(lexer);
    NT_Parser_free(parser);
    return template;

fail:
    if (tokens)
    {
        PyMem_Free(tokens);
        tokens = NULL;
    }

    if (lexer)
    {
        NT_Lexer_free(lexer);
        lexer = NULL;
    }

    if (parser)
    {
        NT_Parser_free(parser);
        parser = NULL;
    }

    if (ast)
    {
        NT_Mem_free(ast);
        ast = NULL;
    }

    return NULL;
}
