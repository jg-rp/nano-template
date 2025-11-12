#ifndef MLPY_TOKENIZE_H
#define MLPY_TOKENIZE_H

#include <Python.h>

/**
 * Tokenize `str`. Return a list of token view instances.
 *
 * This is use for testing our C lexer from Python. It's unlikely to be useful
 * elsewhere.
 *
 * @param self Python module
 */
PyObject *tokenize(PyObject *self, PyObject *str);

#endif
