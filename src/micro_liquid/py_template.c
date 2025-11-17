#include "micro_liquid/py_template.h"

// TODO:
// TODO: Comments
// TODO: Register with dummy return value
// TODO:

/// @brief Render template `self`
/// @param self The template to render
/// @param globals A Python mapping of strings to objects.
///     `Mapping[str, object]`.
/// @param serializer A Python callable used to stringify objects before writing
///     them to the output buffer. `Callable[[object], str]`.
/// @param undefined The Python object used when a variable can not be resolved.
///     `Callable[[str, int, int, int], Undefined]`.
/// @return The result of rendering template `self` as a Python string.
PyObject *MLPY_Template_render(MLPY_TemplateObject *self, PyObject *globals,
                               PyObject *serializer, PyObject *undefined);

PyObject *MLPY_TemplateObject_new(PyObject **nodes, Py_ssize_t node_count);
void MLPY_Object_dealloc(PyObject *self);