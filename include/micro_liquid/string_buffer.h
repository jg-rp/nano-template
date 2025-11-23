#ifndef ML_STRING_BUFFER_H
#define ML_STRING_BUFFER_H

#include "micro_liquid/common.h"

/// @brief Allocate a new empty string buffer.
/// @return A PyList to act as our buffer.
PyObject *StringBuffer_new(void);

/// @brief Append a string to the buffer.
/// @return 0 on success, -1 on failure with an exception set.
int StringBuffer_append(PyObject *self, PyObject *str);

/// @brief Join buffer items into a single string an destroy the buffer.
/// Do not Py_DECREF the buffer after calling this.
/// @return The concatenated string, or NULL on failure.
PyObject *StringBuffer_finish(PyObject *self);

#endif
