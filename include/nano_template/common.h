// SPDX-License-Identifier: MIT

#ifndef NT_COMMON_H
#define NT_COMMON_H

#include <Python.h>
#include <stdbool.h>
#include <stdint.h>

#define NTPY_TODO()                                                           \
    do                                                                        \
    {                                                                         \
        PyErr_Format(PyExc_NotImplementedError,                               \
                     "TODO: not implemented (%s:%d)", __FILE__, __LINE__);    \
        return NULL;                                                          \
    } while (0)

#define NTPY_TODO_I()                                                         \
    do                                                                        \
    {                                                                         \
        PyErr_Format(PyExc_NotImplementedError,                               \
                     "TODO: not implemented (%s:%d)", __FILE__, __LINE__);    \
        return -1;                                                            \
    } while (0)

#define NTPY_DEBUG_PRINT(obj)                                                 \
    do                                                                        \
    {                                                                         \
        PyObject *_repr = PyObject_Repr(obj);                                 \
        if (_repr)                                                            \
        {                                                                     \
            PyObject *_bytes =                                                \
                PyUnicode_AsEncodedString(_repr, "utf-8", "strict");          \
            if (_bytes)                                                       \
            {                                                                 \
                char *_str = PyBytes_AsString(_bytes);                        \
                if (_str)                                                     \
                {                                                             \
                    printf("%s = %s\n", #obj, _str);                          \
                }                                                             \
                Py_DECREF(_bytes);                                            \
            }                                                                 \
            Py_DECREF(_repr);                                                 \
        }                                                                     \
    } while (0)

#endif
