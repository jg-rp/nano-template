#ifndef ML_COMMON_H
#define ML_COMMON_H

#include <Python.h>
#include <stdbool.h>

// TODO: remove PY_ prefix

#define PY_TODO()                                                              \
    do                                                                         \
    {                                                                          \
        PyErr_Format(PyExc_NotImplementedError,                                \
                     "TODO: not implemented (%s:%d)", __FILE__, __LINE__);     \
        return NULL;                                                           \
    } while (0)

#define PY_TODO_I()                                                            \
    do                                                                         \
    {                                                                          \
        PyErr_Format(PyExc_NotImplementedError,                                \
                     "TODO: not implemented (%s:%d)", __FILE__, __LINE__);     \
        return -1;                                                             \
    } while (0)

#endif
