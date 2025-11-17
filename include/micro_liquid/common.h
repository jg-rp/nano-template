#ifndef ML_COMMON_H
#define ML_COMMON_H

#include <Python.h>
#include <stdbool.h>

#define PY_TODO()                                                              \
    do                                                                         \
    {                                                                          \
        PyErr_Format(PyExc_NotImplementedError,                                \
                     "TODO: not implemented (%s:%d)", __FILE__, __LINE__);     \
        return NULL;                                                           \
    } while (0)

#endif
