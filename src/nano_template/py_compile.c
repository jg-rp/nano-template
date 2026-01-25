// SPDX-License-Identifier: MIT

#include "nano_template/py_compile.h"
#include "nano_template/compiler.h"
#include "nano_template/lexer.h"
#include "nano_template/parser.h"
#include "nano_template/py_compiled_template.h"

PyObject *nt_compile(PyObject *Py_UNUSED(self), PyObject *args)
{
    Py_ssize_t token_count = 0;
    NT_Token *tokens = NULL;
    NT_Lexer *lexer = NULL;
    NT_Parser *parser = NULL;

    NT_Mem *ast = NULL;
    NT_Node *root = NULL;
    NT_Compiler *compiler = NULL;
    NT_Code *bytecode = NULL;
    PyObject *template = NULL;
    PyObject *result = NULL;

    PyObject *str;
    PyObject *serializer;
    PyObject *undefined;

    if (!PyArg_ParseTuple(args, "OOO", &str, &serializer, &undefined))
    {
        return NULL;
    }

    if (!PyUnicode_Check(str))
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

    lexer = NT_Lexer_new(str);
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

    parser = NT_Parser_new(ast, str, tokens, token_count);
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

    compiler = NT_Compiler_new();
    if (!compiler)
    {
        goto cleanup;
    }

    if (NT_Compiler_compile(compiler, root) < 0)
    {
        goto cleanup;
    }

    bytecode = NT_Compiler_bytecode(compiler);
    if (!bytecode)
    {
        goto cleanup;
    }

    template = NTPY_CompiledTemplate_new(bytecode, serializer, undefined);
    if (!template)
    {
        goto cleanup;
    }

    result = template;
    template = NULL;

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

    if (compiler)
    {
        NT_Compiler_free(compiler);
        compiler = NULL;
    }

    if (bytecode)
    {
        NT_Code_free(bytecode);
        bytecode = NULL;
    }

    return result;
}