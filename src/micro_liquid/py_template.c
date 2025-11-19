#include "micro_liquid/py_template.h"
#include "micro_liquid/context.h"
#include "micro_liquid/object_list.h"

static PyTypeObject *Template_TypeObject = NULL;

void MLPY_Template_dealloc(PyObject *self)
{
    MLPY_Template *op = (MLPY_Template *)self;

    for (Py_ssize_t i = 0; i < op->node_count; i++)
        ML_Node_dealloc(op->nodes[i]);

    Py_XDECREF(op->str);
    PyMem_Free(op);
}

PyObject *MLPY_Template_new(PyObject *str, ML_Node **nodes,
                            Py_ssize_t node_count)
{

    if (!Template_TypeObject)
    {
        PyErr_SetString(PyExc_RuntimeError, "Template type not initialized");
        return NULL;
    }

    PyObject *obj = PyType_GenericNew(Template_TypeObject, NULL, NULL);
    if (!obj)
        return NULL;

    MLPY_Template *op = (MLPY_Template *)obj;

    Py_INCREF(str);
    op->str = str;
    op->nodes = nodes;
    op->node_count = node_count;
    return obj;
}

// TODO: __str__

/// @brief Render a compiled template into a string.
/// @return Python `str` containing the rendered template on success, or `NULL`
///     on error with a Python exception set.
static PyObject *MLPY_Template_render(PyObject *self, PyObject *args)
{
    MLPY_Template *op = (MLPY_Template *)self;
    PyObject *globals = NULL;
    PyObject *serializer = NULL;
    PyObject *undefined = NULL;
    ML_Context *ctx = NULL;
    ML_ObjList *buf = NULL;
    PyObject *rv = NULL;

    // TODO: or kwargs, remember METH_KEYWORDS bit flag

    if (!PyArg_ParseTuple(args, "OOO", &globals, &serializer, &undefined))
        return NULL;

    // TODO: validate arguments

    ctx = ML_Context_new(op->str, globals, serializer, undefined);
    if (!ctx)
        goto fail;

    buf = ML_ObjList_new();
    if (!buf)
        goto fail;

    for (Py_ssize_t i = 0; i < op->node_count; i++)
    {
        if (ML_Node_render(op->nodes[i], ctx, buf) < 0)
            goto fail;
    }

    rv = ML_ObjList_join(buf);
    if (!rv)
        goto fail;

    ML_Context_dealloc(ctx);
    ML_ObjList_destroy(buf);
    return rv;

fail:
    if (ctx)
        ML_Context_dealloc(ctx);
    if (buf)
        ML_ObjList_destroy(buf);
    Py_XDECREF(rv);
    return NULL;
}

static PyMethodDef Template_methods[] = {{"render",
                                          (PyCFunction)MLPY_Template_render,
                                          METH_VARARGS, "Render the template"},
                                         {NULL, NULL, 0, NULL}};

static PyType_Slot Template_slots[] = {
    {Py_tp_doc, "Compiled micro template"},
    {Py_tp_dealloc, (void *)MLPY_Template_dealloc},
    {Py_tp_methods, Template_methods},
    {0, NULL}};

static PyType_Spec Template_spec = {
    .name = "micro_liquid.Template",
    .basicsize = sizeof(MLPY_Template),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Template_slots,
};

int ml_register_template_type(PyObject *module)
{
    PyObject *type_obj = PyType_FromSpec(&Template_spec);
    if (!type_obj)
        return -1;

    Template_TypeObject = (PyTypeObject *)type_obj;

    if (PyModule_AddObject(module, "Template", type_obj) < 0)
    {
        Py_DECREF(type_obj);
        Template_TypeObject = NULL;
        return -1;
    }

    return 0;
}