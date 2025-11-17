#ifndef ML_TEMPLATE_H
#define ML_TEMPLATE_H

#include "micro_liquid/common.h"
#include "micro_liquid/node.h"

/// @brief A compiled template, ready to be rendered.
typedef struct MLPY_TemplateObject
{
    PyObject_HEAD PyObject *str;
    ML_Node **nodes;
    Py_ssize_t node_count;
} MLPY_TemplateObject;

/// @brief Allocate and initialize a new MLPY_TemplateObject taking ownership
/// of `nodes`.
/// @return Newly allocated MLPY_TemplateObject*, or NULL on memory error.
PyObject *MLPY_TemplateObject_new(PyObject *str, PyObject **nodes,
                                  Py_ssize_t node_count);

void MLPY_Object_dealloc(PyObject *self);

/// @brief Render a compiled template into a string.
/// @param self The compiled template to render.
/// @param globals Mapping object containing variable context (e.g., `dict`).
/// @param serializer Callable used to convert Python objects to strings.
/// @param undefined Type object (subclass of `Undefined`) used when a variable
///     cannot be resolved.
/// @return Python `str` containing the rendered template on success, or `NULL`
///     on error with a Python exception set.
PyObject *MLPY_Template_render(PyObject *self, PyObject *args);

int ml_register_template_type(PyObject *module);

#endif
