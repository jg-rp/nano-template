// SPDX-License-Identifier: MIT

#include "nano_template/py_token_view.h"
#include "nano_template/token.h"

static PyTypeObject *TokenView_TypeObject = NULL;

PyObject *NTPY_TokenView_new(PyObject *source, Py_ssize_t start,
                             Py_ssize_t end, int kind)
{
    if (!TokenView_TypeObject)
    {
        PyErr_SetString(PyExc_RuntimeError, "TokenView type not initialized");
        return NULL;
    }

    PyObject *obj = PyType_GenericNew(TokenView_TypeObject, NULL, NULL);
    if (!obj)
    {
        return NULL;
    }

    NTPY_TokenViewObject *op = (NTPY_TokenViewObject *)obj;

    if (!op)
    {
        return NULL;
    }

    Py_INCREF(source);
    op->source = source;
    op->start = start;
    op->end = end;
    op->kind = kind;

    return obj;
}

static PyObject *TokenView_text(PyObject *self, void *Py_UNUSED(closure))
{
    NTPY_TokenViewObject *op = (NTPY_TokenViewObject *)self;
    return PyUnicode_Substring(op->source, op->start, op->end);
}

static PyObject *TokenView_kind(PyObject *self, void *Py_UNUSED(closure))
{
    NTPY_TokenViewObject *op = (NTPY_TokenViewObject *)self;
    return PyLong_FromLong(op->kind);
}

static PyObject *TokenView_start(PyObject *self, void *Py_UNUSED(closure))
{
    NTPY_TokenViewObject *op = (NTPY_TokenViewObject *)self;
    return PyLong_FromSsize_t(op->start);
}

static PyObject *TokenView_end(PyObject *self, void *Py_UNUSED(closure))
{
    NTPY_TokenViewObject *op = (NTPY_TokenViewObject *)self;
    return PyLong_FromSsize_t(op->end);
}

static void TokenView_free(PyObject *self)
{
    NTPY_TokenViewObject *op = (NTPY_TokenViewObject *)self;
    Py_XDECREF(op->source);
    PyObject_Free(op);
}

static PyObject *TokenView_repr(PyObject *self)
{
    NTPY_TokenViewObject *op = (NTPY_TokenViewObject *)self;
    PyObject *text = PyUnicode_Substring(op->source, op->start, op->end);
    if (!text)
    {
        return NULL;
    }
    PyObject *repr = PyUnicode_FromFormat("<TokenView kind=%s, text=%R>",
                                          NT_TokenKind_str(op->kind), text);
    Py_DECREF(text);
    return repr;
}

static PyGetSetDef TokenView_getset[] = {
    {"start", TokenView_start, NULL, "start index", NULL},
    {"end", TokenView_end, NULL, "end index", NULL},
    {"text", TokenView_text, NULL, "substring text", NULL},
    {"kind", TokenView_kind, NULL, "token kind", NULL},
    {NULL, NULL, NULL, NULL, NULL}};

static PyType_Slot TokenView_slots[] = {
    {Py_tp_doc, "Lightweight token view into source text"},
    {Py_tp_free, (void *)TokenView_free},
    {Py_tp_repr, (void *)TokenView_repr},
    {Py_tp_getset, (void *)TokenView_getset},
    {0, NULL}};

static PyType_Spec TokenView_spec = {
    .name = "nano_template.TokenView",
    .basicsize = sizeof(NTPY_TokenViewObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = TokenView_slots,
};

int nt_register_token_view_type(PyObject *module)
{
    PyObject *type_obj = PyType_FromSpec(&TokenView_spec);
    if (!type_obj)
    {
        return -1;
    }

    /* Store type object for future factories */
    TokenView_TypeObject = (PyTypeObject *)type_obj;

    if (PyModule_AddObject(module, "TokenView", type_obj) < 0)
    {
        Py_DECREF(type_obj);
        TokenView_TypeObject = NULL;
        return -1;
    }

    return 0;
}