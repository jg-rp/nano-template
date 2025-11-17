#ifndef MLPY_TOKEN_VIEW_H
#define MLPY_TOKEN_VIEW_H

#include <Python.h>

/**
 * A convenient Python type exposing tokens for testing.
 */
typedef struct
{
    PyObject_HEAD PyObject *source;
    Py_ssize_t start;
    Py_ssize_t end;
    int kind;
} MLPY_TokenViewObject;

PyObject *MLPY_TokenView_new(PyObject *source, Py_ssize_t start, Py_ssize_t end,
                             int kind);

/* Initialization: adds TokenView type to the module */
int ml_register_token_view_type(PyObject *module);

#endif
