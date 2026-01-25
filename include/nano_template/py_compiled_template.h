// SPDX-License-Identifier: MIT

#ifndef NT_COMPILED_TEMPLATE_H
#define NT_COMPILED_TEMPLATE_H

#include "nano_template/common.h"
#include "nano_template/vm.h"

typedef struct NTPY_CompiledTemplate
{
    PyObject_HEAD NT_VM *vm;
} NTPY_CompiledTemplate;

/// @brief Allocate and initialize a new compiled template.
/// @param code Bytecode as returned from `NT_Compiler_bytecode()`.
/// @param serializer A Python callable for serializing objects for output.
/// @param undefined An "undefined" type representing template variables that
/// can't be resolved.
/// @return The newly allocated compiled template, or NULL on error with an
/// exception set.
PyObject *NTPY_CompiledTemplate_new(NT_Code *code, PyObject *serializer,
                                    PyObject *undefined);

void NTPY_CompiledTemplate_free(PyObject *self);

/// @brief Render a compiled template using user data from `data`.
/// @return The rendered template as a Python str, or NULL on error with an
/// exception set.
PyObject *NTPY_CompiledTemplate_render(PyObject *self, PyObject *data);

int nt_register_compiled_template_type(PyObject *module);
#endif
