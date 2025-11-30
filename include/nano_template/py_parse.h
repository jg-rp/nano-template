#ifndef NTPY_PARSE_H
#define NTPY_PARSE_H

#include "nano_template/common.h"

/// @brief Parse template `src`.
/// @param src A Python string.
/// @return A new reference to a NTPY_Template, or NULL on error with an
/// exception set.
PyObject *parse(PyObject *self, PyObject *src);

#endif
