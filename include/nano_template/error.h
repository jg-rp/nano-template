// SPDX-License-Identifier: MIT

#ifndef NT_ERROR_H
#define NT_ERROR_H

#include "nano_template/common.h"
#include "nano_template/token.h"
#include <stdarg.h>

/// @brief Set a RuntimeError with start and stop index from `token`.
static void *nt_parser_error(const NT_Token *token, const char *fmt, ...)
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
        goto cleanup;
    }

    exc_instance =
        PyObject_CallFunctionObjArgs(PyExc_RuntimeError, msg_obj, NULL);
    if (!exc_instance)
    {
        goto cleanup;
    }

    start_obj = PyLong_FromSsize_t(token->start);
    if (!start_obj)
    {
        goto cleanup;
    }

    end_obj = PyLong_FromSsize_t(token->end);
    if (!end_obj)
    {
        goto cleanup;
    }

    if (PyObject_SetAttrString(exc_instance, "start_index", start_obj) < 0 ||
        PyObject_SetAttrString(exc_instance, "stop_index", end_obj) < 0)
    {
        goto cleanup;
    }

    PyErr_SetObject(PyExc_RuntimeError, exc_instance);

cleanup:
    Py_XDECREF(start_obj);
    Py_XDECREF(end_obj);
    Py_XDECREF(msg_obj);
    Py_XDECREF(exc_instance);
    return NULL;
}

#endif
