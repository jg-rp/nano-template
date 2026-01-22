// SPDX-License-Identifier: MIT

#ifndef NTPY_BYTECODE_VIEW_H
#define NTPY_BYTECODE_VIEW_H

#include "nano_template/common.h"
#include "nano_template/compiler.h"

/// @brief Expose bytecode instructions and constants to Python for testing.
typedef struct NTPY_BytecodeViewObject
{
    PyObject_HEAD PyObject *instructions; // PyBytes
    PyObject *constant_pool;              // PyList
} NTPY_BytecodeViewObject;

PyObject *NTPY_BytecodeView_new(NT_Code *code);

/// @brief Parse and compile template source string `str`.
/// @return A bytecode view suitable for testing.
PyObject *NTPY_bytecode(PyObject *Py_UNUSED(self), PyObject *str);

PyObject *NTPY_bytecode_definitions(PyObject *Py_UNUSED(self));

int nt_register_bytecode_view_type(PyObject *module);

#endif
