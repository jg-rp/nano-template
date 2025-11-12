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

    MLPY_TokenViewObject *_self =
        PyObject_New(MLPY_TokenViewObject, TokenView_TypeObject);
    if (!_self)
        return NULL;

    Py_INCREF(source);
    _self->source = source;
    _self->start = start;
    _self->end = end;
    _self->kind = kind;

    return (PyObject *)_self;
}

static PyObject *TokenView_text(PyObject *self, void *Py_UNUSED(closure))
{
    MLPY_TokenViewObject *_self = (MLPY_TokenViewObject *)self;
    return PyUnicode_Substring(_self->source, _self->start, _self->end);
}

static PyObject *TokenView_kind(PyObject *self, void *Py_UNUSED(closure))
{
    MLPY_TokenViewObject *_self = (MLPY_TokenViewObject *)self;
    return PyLong_FromLong(_self->kind);
}

static PyObject *TokenView_start(PyObject *self, void *Py_UNUSED(closure))
{
    MLPY_TokenViewObject *_self = (MLPY_TokenViewObject *)self;
    return PyLong_FromSsize_t(_self->start);
}

static PyObject *TokenView_end(PyObject *self, void *Py_UNUSED(closure))
{
    MLPY_TokenViewObject *_self = (MLPY_TokenViewObject *)self;
    return PyLong_FromSsize_t(_self->end);
}

static void TokenView_dealloc(PyObject *self)
{
    MLPY_TokenViewObject *_self = (MLPY_TokenViewObject *)self;
    Py_XDECREF(_self->source);
    PyObject_Free(_self);
}

static PyObject *TokenView_repr(PyObject *self)
{
    MLPY_TokenViewObject *_self = (MLPY_TokenViewObject *)self;
    PyObject *text =
        PyUnicode_Substring(_self->source, _self->start, _self->end);
    if (!text)
        return NULL;
    PyObject *repr =
        PyUnicode_FromFormat("<TokenView kind=%s, text=%R>",
                             ML_TokenKind_to_string(_self->kind), text);
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

int register_token_view_type(PyObject *module)
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