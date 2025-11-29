#ifndef ML_ERROR_H
#define ML_ERROR_H

#include <stdarg.h>
#include "micro_liquid/common.h"
#include "micro_liquid/token.h"

// TODO: rename with prefix
static void *parser_error(ML_Token *token, const char *fmt, ...)
{
    PyObject *exc_instance = NULL;
    PyObject *start_obj = NULL;
    PyObject *end_obj = NULL;
    PyObject *msg_obj = NULL;

    va_list vargs;
    va_start(vargs, fmt);
    msg_obj = PyUnicode_FromFormatV(fmt, vargs);
    va_end(vargs);

    if (!msg_obj)
    {
        goto fail;
    }

    exc_instance =
        PyObject_CallFunctionObjArgs(PyExc_RuntimeError, msg_obj, NULL);
    if (!exc_instance)
    {
        goto fail;
    }

    start_obj = PyLong_FromSsize_t(token->start);
    if (!start_obj)
    {
        goto fail;
    }

    end_obj = PyLong_FromSsize_t(token->end);
    if (!end_obj)
    {
        goto fail;
    }

    if (PyObject_SetAttrString(exc_instance, "start_index", start_obj) < 0 ||
        PyObject_SetAttrString(exc_instance, "stop_index", end_obj) < 0)
    {
        goto fail;
    }

    Py_DECREF(start_obj);
    Py_DECREF(end_obj);
    Py_DECREF(msg_obj);

    PyErr_SetObject(PyExc_RuntimeError, exc_instance);
    Py_DECREF(exc_instance);
    return NULL;

fail:
    Py_XDECREF(start_obj);
    Py_XDECREF(end_obj);
    Py_XDECREF(msg_obj);
    Py_XDECREF(exc_instance);
    return NULL;
}

#endif
