// SPDX-License-Identifier: MIT

#ifndef NT_TEMPLATE_H
#define NT_TEMPLATE_H

#include "nano_template/allocator.h"
#include "nano_template/common.h"
#include "nano_template/node.h"

typedef struct NTPY_Template
{
    PyObject_HEAD

        PyObject *str;
    NT_Node *root;
    NT_Mem *ast;

    PyObject *serializer; // Callable[[object], str]
    PyObject *undefined;  // Type[Undefined]
} NTPY_Template;

/// @brief Allocate and initialize a new NTPY_Template.
/// @return The new template, or NULL on failure with an exception set.
PyObject *NTPY_Template_new(PyObject *str, NT_Node *root, NT_Mem *ast,
                            PyObject *serializer, PyObject *undefined);

void NTPY_Template_free(PyObject *self);

int nt_register_template_type(PyObject *module);

#endif
