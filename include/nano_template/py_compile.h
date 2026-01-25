// SPDX-License-Identifier: MIT

#ifndef NTPY_COMPILE_H
#define NTPY_COMPILE_H

#include "nano_template/common.h"

/// @brief Parse and compile the argument string as a template.
/// @return A new reference to a NTPY_CompiledTemplate, or NULL on error with
/// an exception set.
PyObject *nt_compile(PyObject *self, PyObject *args);

#endif
