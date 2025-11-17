#include "micro_liquid/py_template.h"
#include "micro_liquid/context.h"
#include "micro_liquid/object_list.h"

PyObject *MLPY_Template_render(PyObject *self, PyObject *args)
{
    MLPY_TemplateObject *this = (MLPY_TemplateObject *)self;
    PyObject *globals = NULL;
    PyObject *serializer = NULL;
    PyObject *undefined = NULL;
    ML_Context *ctx = NULL;
    ML_ObjList *buf = NULL;
    PyObject *rv = NULL;

    if (!PyArg_ParseTuple(args, "OOO", &globals, &serializer, &undefined))
        return NULL;

    // TODO: validate arguments

    ctx = ML_Context_new(this->str, globals, serializer, undefined);
    if (!ctx)
        goto fail;

    buf = ML_ObjList_new();
    if (!buf)
        goto fail;

    for (Py_ssize_t i = 0; i < this->node_count; i++)
    {
        ML_Node_render(this->nodes[i], ctx, buf);
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
