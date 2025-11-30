#ifndef NT_TEMPLATE_H
#define NT_TEMPLATE_H

#include "nano_template/allocator.h"
#include "nano_template/common.h"
#include "nano_template/node.h"

/// @brief A compiled template, ready to be rendered.
typedef struct NTPY_Template
{
    PyObject_HEAD PyObject *str;
    NT_Node *root;
    NT_Mem *ast;
} NTPY_Template;

// TODO: doc comments

PyObject *NTPY_Template_new(PyObject *str, NT_Node *root, NT_Mem *ast);

void NTPY_Template_dealloc(PyObject *self);

int nt_register_template_type(PyObject *module);

#endif
