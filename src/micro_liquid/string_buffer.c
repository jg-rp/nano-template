#include "micro_liquid/string_buffer.h"

PyObject *StringBuffer_new()
{
    return PyList_New(0);
}

int StringBuffer_append(PyObject *self, PyObject *str)
{
    return PyList_Append(self, str);
}

PyObject *StringBuffer_finish(PyObject *self)
{
    if (!self)
        return NULL;

    PyObject *empty = PyUnicode_FromString("");
    if (!empty)
    {
        Py_DECREF(self);
        return NULL;
    }

    PyObject *result = PyUnicode_Join(empty, self);

    Py_DECREF(empty);
    Py_DECREF(self); // TODO: maybe not
    return result;
}