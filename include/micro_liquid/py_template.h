#ifndef ML_TEMPLATE_H
#define ML_TEMPLATE_H

#include "micro_liquid/allocator.h"
#include "micro_liquid/common.h"
#include "micro_liquid/node.h"

/// @brief A compiled template, ready to be rendered.
typedef struct MLPY_Template
{
    PyObject_HEAD PyObject *str;
    ML_Node *root;
    ML_Mem *ast;
} MLPY_Template;

// TODO: doc comments

PyObject *MLPY_Template_new(PyObject *str, ML_Node *root, ML_Mem *ast);

void MLPY_Template_dealloc(PyObject *self);

int ml_register_template_type(PyObject *module);

#endif
