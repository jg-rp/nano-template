#include "micro_liquid/py_tokenize.h"
#include "micro_liquid/lexer.h"
#include "micro_liquid/py_token_view.h"
#include "micro_liquid/token.h"

PyObject *tokenize(PyObject *Py_UNUSED(self), PyObject *str)
{
    ML_Lexer *lexer = ML_Lexer_new(str);
    if (!lexer)
        return NULL;

    Py_ssize_t token_count = 0;
    ML_Token **tokens = ML_Lexer_scan(lexer, &token_count);
    if (!tokens)
    {
        ML_Lexer_delete(lexer);
        return NULL;
    }

    PyObject *list = PyList_New(token_count);
    if (!list)
        goto fail;

    for (Py_ssize_t i = 0; i < token_count; i++)
    {
        ML_Token *token = tokens[i];
        PyObject *view =
            MLPY_TokenView_new(str, token->start, token->end, token->kind);

        if (!view)
            goto fail;

        if (PyList_SetItem(list, i, view) < 0)
        {
            Py_DECREF(view);
            goto fail;
        }

        PyMem_Free(token);
    }

    PyMem_Free(tokens);
    ML_Lexer_delete(lexer);
    return list;

fail:
    if (tokens)
    {
        for (Py_ssize_t j = 0; j < token_count; j++)
        {
            if (tokens[j])
                PyMem_Free(tokens[j]);
        }
        PyMem_Free(tokens);
    }
    ML_Lexer_delete(lexer);
    Py_XDECREF(list);
    return NULL;
}
