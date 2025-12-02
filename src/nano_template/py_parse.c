// SPDX-License-Identifier: MIT

#include "nano_template/py_parse.h"
#include "nano_template/allocator.h"
#include "nano_template/lexer.h"
#include "nano_template/parser.h"
#include "nano_template/py_template.h"

PyObject *parse(PyObject *Py_UNUSED(self), PyObject *args)
{
    Py_ssize_t token_count = 0;
    NT_Token *tokens = NULL;
    NT_Lexer *lexer = NULL;
    NT_Parser *parser = NULL;

    NT_Mem *ast = NULL;
    NT_Node *root = NULL;
    PyObject *template = NULL;

    PyObject *src;
    PyObject *serializer;
    PyObject *undefined;

    if (!PyArg_ParseTuple(args, "OOO", &src, &serializer, &undefined))
    {
        return NULL;
    }

    if (!PyUnicode_Check(src))
    {
        PyErr_SetString(PyExc_TypeError, "parse() argument must be a string");
        goto cleanup;
    }

    if (!PyCallable_Check(serializer))
    {
        PyErr_SetString(PyExc_TypeError, "serializer must be callable");
        goto cleanup;
    }

    if (!PyCallable_Check(undefined))
    {
        PyErr_SetString(PyExc_TypeError,
                        "undefined must be a type (callable)");
        goto cleanup;
    }

    lexer = NT_Lexer_new(src);
    if (!lexer)
    {
        goto cleanup;
    }

    tokens = NT_Lexer_scan(lexer, &token_count);
    if (!tokens)
    {
        goto cleanup;
    }

    ast = NT_Mem_new();
    if (!ast)
    {
        goto cleanup;
    }

    parser = NT_Parser_new(ast, src, tokens, token_count);
    if (!parser)
    {
        goto cleanup;
    }

    tokens = NULL;

    root = NT_Parser_parse_root(parser);
    if (!root)
    {
        goto cleanup;
    }

    template = NTPY_Template_new(src, root, ast, serializer, undefined);
    if (!template)
    {
        goto cleanup;
    }

    root = NULL;
    ast = NULL;

cleanup:
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
        // `root` is allocated and freed by `ast`
        root = NULL;
    }

    return template;
}
