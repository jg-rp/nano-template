// SPDX-License-Identifier: MIT

#include "nano_template/py_template.h"
#include "nano_template/context.h"
#include "nano_template/string_buffer.h"

static PyTypeObject *Template_TypeObject = NULL;

void NTPY_Template_free(PyObject *self)
{
    NTPY_Template *op = (NTPY_Template *)self;
    NT_Mem_free(op->ast);
    Py_XDECREF(op->str);
    Py_XDECREF(op->serializer);
    Py_XDECREF(op->undefined);
    PyObject_Free(op);
}

PyObject *NTPY_Template_new(PyObject *str, NT_Node *root, NT_Mem *ast,
                            PyObject *serializer, PyObject *undefined)
{

    if (!Template_TypeObject)
    {
        PyErr_SetString(PyExc_RuntimeError, "Template type not initialized");
        return NULL;
    }

    PyObject *obj = PyType_GenericNew(Template_TypeObject, NULL, NULL);
    if (!obj)
    {
        return NULL;
    }

    NTPY_Template *op = (NTPY_Template *)obj;

    Py_INCREF(str);
    Py_INCREF(serializer);
    Py_INCREF(undefined);

    op->str = str;
    op->root = root;
    op->ast = ast;
    op->serializer = serializer;
    op->undefined = undefined;
    return obj;
}

/// @brief Render template with data from `globals`.
/// @param globals dict[str, Any]
/// @return The rendered string on success, or `NULL` on error with an
/// exception set.
static PyObject *NTPY_Template_render(PyObject *self, PyObject *globals)
{
    NTPY_Template *op = (NTPY_Template *)self;
    NT_RenderContext *ctx = NULL;
    PyObject *buf = NULL;
    PyObject *rv = NULL;

    ctx =
        NT_RenderContext_new(op->str, globals, op->serializer, op->undefined);
    if (!ctx)
    {
        goto fail;
    }

    buf = StringBuffer_new();
    if (!buf)
    {
        goto fail;
    }

    NT_Node *root = op->root;
    NT_NodePage *page = root->head;

    while (page)
    {
        for (Py_ssize_t i = 0; i < page->count; i++)
        {
            if (NT_Node_render(page->nodes[i], ctx, buf) < 0)
            {
                goto fail;
            }
        }

        page = page->next;
    }

    rv = StringBuffer_finish(buf);
    if (!rv)
    {
        goto fail;
    }

    NT_RenderContext_free(ctx);
    return rv;

fail:
    if (ctx)
    {
        NT_RenderContext_free(ctx);
    }
    Py_XDECREF(buf);
    Py_XDECREF(rv);
    return NULL;
}

static PyMethodDef Template_methods[] = {
    {"render", NTPY_Template_render, METH_O, "Render the template"},
    {NULL, NULL, 0, NULL}};

static PyType_Slot Template_slots[] = {
    {Py_tp_doc, "Compiled template"},
    {Py_tp_free, (void *)NTPY_Template_free},
    {Py_tp_methods, Template_methods},
    {0, NULL}};

static PyType_Spec Template_spec = {
    .name = "nano_template.Template",
    .basicsize = sizeof(NTPY_Template),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Template_slots,
};

int nt_register_template_type(PyObject *module)
{
    PyObject *type_obj = PyType_FromSpec(&Template_spec);
    if (!type_obj)
    {
        return -1;
    }

    Template_TypeObject = (PyTypeObject *)type_obj;

    if (PyModule_AddObject(module, "Template", type_obj) < 0)
    {
        Py_DECREF(type_obj);
        Template_TypeObject = NULL;
        return -1;
    }

    return 0;
}