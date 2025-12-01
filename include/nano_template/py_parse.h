// SPDX-License-Identifier: MIT

#ifndef NTPY_PARSE_H
#define NTPY_PARSE_H

#include "nano_template/common.h"

/// @brief Parse argument string as a template.
/// @return A new reference to a NTPY_Template, or NULL on error with an
/// exception set.
PyObject *parse(PyObject *self, PyObject *args);

#endif
