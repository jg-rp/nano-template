#include "micro_liquid/py_template.h"

// TODO:
// TODO: Comments
// TODO: Register with dummy return value
// TODO:

/**
 * Render a template.
 * @param globals Mapping[str, object]
 * @param serializer Callable[[object], str]
 * @param undefined Type[Undefined]
 */
PyObject *MLPY_Template_render(MLPY_TemplateObject *self, PyObject *globals,
                               PyObject *serializer, PyObject *undefined);

PyObject *MLPY_TemplateObject_new(PyObject **nodes, Py_ssize_t node_count);
void MLPY_Object_dealloc(PyObject *self);