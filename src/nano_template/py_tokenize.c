// SPDX-License-Identifier: MIT

#include "nano_template/py_tokenize.h"
#include "nano_template/lexer.h"
#include "nano_template/py_token_view.h"
#include "nano_template/token.h"

PyObject *tokenize(PyObject *Py_UNUSED(self), PyObject *str)
{
    NT_Lexer *lexer = NULL;
    NT_Token *tokens = NULL;
    PyObject *list = NULL;
    PyObject *view = NULL;
    PyObject *result = NULL;

    lexer = NT_Lexer_new(str);
    if (!lexer)
    {
        return NULL;
    }

    Py_ssize_t token_count = 0;

    tokens = NT_Lexer_scan(lexer, &token_count);
    if (!tokens)
    {
        NT_Lexer_free(lexer);
        return NULL;
    }

    list = PyList_New(token_count);
    if (!list)
    {
        goto cleanup;
    }

    for (Py_ssize_t i = 0; i < token_count; i++)
    {
        NT_Token token = tokens[i];
        view = NTPY_TokenView_new(str, token.start, token.end, token.kind);

        if (!view)
        {
            goto cleanup;
        }

        if (PyList_SetItem(list, i, view) < 0)
        {
            Py_DECREF(view);
            view = NULL;
            goto cleanup;
        }

        view = NULL;
    }

    result = Py_NewRef(list);

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

    Py_XDECREF(list);
    return result;
}
