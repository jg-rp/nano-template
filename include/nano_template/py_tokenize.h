// SPDX-License-Identifier: MIT

#ifndef NTPY_TOKENIZE_H
#define NTPY_TOKENIZE_H

#include "nano_template/common.h"

/// @brief Tokenize `str`. Used for testing.
/// @return A new reference to a list of NT_TokenView instances, or NULL on
/// error with an exception set.
PyObject *tokenize(PyObject *self, PyObject *str);

#endif
