#ifndef ML_TEMPLATE_H
#define ML_TEMPLATE_H

#include "micro_liquid/common.h"
#include "micro_liquid/node.h"

typedef struct MLPY_TemplateObject
{
    PyObject_HEAD ML_Node **nodes;
    Py_ssize_t node_count;
} MLPY_TemplateObject;

int ml_register_template_type(PyObject *module);

#endif
