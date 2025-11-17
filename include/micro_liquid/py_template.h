#ifndef ML_TEMPLATE_H
#define ML_TEMPLATE_H

#include "micro_liquid/common.h"
#include "micro_liquid/node.h"

/// @brief A compiled template, ready to be rendered.
typedef struct MLPY_Template
{
    PyObject_HEAD PyObject *str;
    ML_Node **nodes;
    Py_ssize_t node_count;
} MLPY_Template;

int ml_register_template_type(PyObject *module);

#endif
