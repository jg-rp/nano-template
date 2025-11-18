#ifndef MLPY_TOKENIZE_H
#define MLPY_TOKENIZE_H

#include "micro_liquid/common.h"

/// @brief Tokenize `str`. Used for testing.
/// @param src A Python string.
/// @return A new reference to a list of ML_TokenView instances, or NULL on
/// error with an exception set.
PyObject *tokenize(PyObject *self, PyObject *str);

#endif
