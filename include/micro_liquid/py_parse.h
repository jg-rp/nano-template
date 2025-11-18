#ifndef MLPY_PARSE_H
#define MLPY_PARSE_H

#include "micro_liquid/common.h"

/// @brief Parse micro template `src`.
/// @param src A Python string.
/// @return A new reference to a MLPY_Template, or NULL on error with an
/// exception set.
PyObject *parse(PyObject *self, PyObject *src);

#endif
