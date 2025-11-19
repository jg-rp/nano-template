#include "micro_liquid/py_token_view.h"
#include "micro_liquid/token.h"

static PyTypeObject *TokenView_TypeObject = NULL;

PyObject *MLPY_TokenView_new(PyObject *source, Py_ssize_t start, Py_ssize_t end,
                             int kind)
{
    if (!TokenView_TypeObject)
    {
        PyErr_SetString(PyExc_RuntimeError, "TokenView type not initialized");
        return NULL;
    }

    PyObject *obj = PyType_GenericNew(TokenView_TypeObject, NULL, NULL);
    if (!obj)
        return NULL;

    MLPY_TokenViewObject *op = (MLPY_TokenViewObject *)obj;

    if (!op)
        return NULL;

    Py_INCREF(source);
    op->source = source;
    op->start = start;
    op->end = end;
    op->kind = kind;

    return obj;
}

static PyObject *TokenView_text(PyObject *self, void *Py_UNUSED(closure))
{
    MLPY_TokenViewObject *op = (MLPY_TokenViewObject *)self;
    return PyUnicode_Substring(op->source, op->start, op->end);
}

static PyObject *TokenView_kind(PyObject *self, void *Py_UNUSED(closure))
{
    MLPY_TokenViewObject *op = (MLPY_TokenViewObject *)self;
    return PyLong_FromLong(op->kind);
}

static PyObject *TokenView_start(PyObject *self, void *Py_UNUSED(closure))
{
    MLPY_TokenViewObject *op = (MLPY_TokenViewObject *)self;
    return PyLong_FromSsize_t(op->start);
}

static PyObject *TokenView_end(PyObject *self, void *Py_UNUSED(closure))
{
    MLPY_TokenViewObject *op = (MLPY_TokenViewObject *)self;
    return PyLong_FromSsize_t(op->end);
}

static void TokenView_dealloc(PyObject *self)
{
    MLPY_TokenViewObject *op = (MLPY_TokenViewObject *)self;
    Py_XDECREF(op->source);
    PyObject_Free(op);
}

static PyObject *TokenView_repr(PyObject *self)
{
    MLPY_TokenViewObject *op = (MLPY_TokenViewObject *)self;
    PyObject *text = PyUnicode_Substring(op->source, op->start, op->end);
    if (!text)
        return NULL;
    PyObject *repr = PyUnicode_FromFormat("<TokenView kind=%s, text=%R>",
                                          ML_TokenKind_str(op->kind), text);
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
    {Py_tp_dealloc, (void *)TokenView_dealloc},
    {Py_tp_repr, (void *)TokenView_repr},
    {Py_tp_getset, (void *)TokenView_getset},
    {0, NULL}};

static PyType_Spec TokenView_spec = {
    .name = "micro_liquid.TokenView",
    .basicsize = sizeof(MLPY_TokenViewObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = TokenView_slots,
};

int ml_register_token_view_type(PyObject *module)
{
    PyObject *type_obj = PyType_FromSpec(&TokenView_spec);
    if (!type_obj)
        return -1;

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