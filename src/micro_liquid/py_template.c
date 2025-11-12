#include "micro_liquid/py_template.h"

// TODO:

/**
 * Render a template.
 * @param scope Mapping[str, object]
 * @param serializer Callable[[object], str]
 * @param undefined Type[Undefined]
 */
PyObject *ML_Template_render(MLPY_TemplateObject *self, PyObject *scope,
                             PyObject *serializer, PyObject *undefined);

PyObject *MLPY_TemplateObject_init(PyObject *str);
void MLPY_Object_dealloc(PyObject *self);