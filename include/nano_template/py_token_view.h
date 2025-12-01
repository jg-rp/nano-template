// SPDX-License-Identifier: MIT

#ifndef NTPY_TOKEN_VIEW_H
#define NTPY_TOKEN_VIEW_H

#include "nano_template/common.h"

/// @brief A convenient Python type exposing tokens for testing.
typedef struct
{
    PyObject_HEAD PyObject *source;
    Py_ssize_t start;
    Py_ssize_t end;
    int kind;
} NTPY_TokenViewObject;

PyObject *NTPY_TokenView_new(PyObject *source, Py_ssize_t start,
                             Py_ssize_t end, int kind);

/* Initialization: adds TokenView type to the module */
int nt_register_token_view_type(PyObject *module);

#endif
