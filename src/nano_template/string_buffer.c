// SPDX-License-Identifier: MIT

#include "nano_template/string_buffer.h"

PyObject *StringBuffer_new()
{
    return PyList_New(0);
}

int StringBuffer_append(PyObject *sb, PyObject *str)
{
    return PyList_Append(sb, str);
}

PyObject *StringBuffer_finish(PyObject *sb)
{
    if (!sb)
    {
        return NULL;
    }

    PyObject *empty = PyUnicode_FromString("");
    if (!empty)
    {
        Py_DECREF(sb);
        return NULL;
    }

    PyObject *result = PyUnicode_Join(empty, sb);

    Py_DECREF(empty);
    Py_DECREF(sb);
    return result;
}