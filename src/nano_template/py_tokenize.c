#include "nano_template/py_tokenize.h"
#include "nano_template/lexer.h"
#include "nano_template/py_token_view.h"
#include "nano_template/token.h"

PyObject *tokenize(PyObject *Py_UNUSED(self), PyObject *str)
{
    // TODO: declare and use goto fail more
    NT_Lexer *lexer = NT_Lexer_new(str);
    if (!lexer)
    {
        return NULL;
    }

    Py_ssize_t token_count = 0;
    NT_Token *tokens = NT_Lexer_scan(lexer, &token_count);
    if (!tokens)
    {
        NT_Lexer_free(lexer);
        return NULL;
    }

    PyObject *list = PyList_New(token_count);
    if (!list)
    {
        goto fail;
    }

    for (Py_ssize_t i = 0; i < token_count; i++)
    {
        NT_Token token = tokens[i];
        PyObject *view =
            NTPY_TokenView_new(str, token.start, token.end, token.kind);

        if (!view)
        {
            goto fail;
        }

        if (PyList_SetItem(list, i, view) < 0)
        {
            Py_DECREF(view);
            goto fail;
        }
    }

    PyMem_Free(tokens);
    NT_Lexer_free(lexer);
    return list;

fail:
    if (tokens)
    {
        PyMem_Free(tokens);
        tokens = NULL;
    }
    NT_Lexer_free(lexer);
    Py_XDECREF(list);
    return NULL;
}
