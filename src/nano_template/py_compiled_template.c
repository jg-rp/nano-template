// SPDX-License-Identifier: MIT

#include "nano_template/py_compiled_template.h"

static PyTypeObject *CompiledTemplate_TypeObject = NULL;

PyObject *NTPY_CompiledTemplate_new(NT_Code *code, PyObject *serializer,
                                    PyObject *undefined)
{
    if (!CompiledTemplate_TypeObject)
    {
        PyErr_SetString(PyExc_RuntimeError,
                        "CompiledTemplate type not initialized");
        return NULL;
    }

    PyObject *obj = PyType_GenericNew(CompiledTemplate_TypeObject, NULL, NULL);
    if (!obj)
    {
        return NULL;
    }

    NT_VM *vm = NT_VM_new(code, serializer, undefined);
    if (!vm)
    {
        Py_DECREF(obj);
        return NULL;
    }

    NTPY_CompiledTemplate *op = (NTPY_CompiledTemplate *)obj;

    op->vm = vm;
    return obj;
}

void NTPY_CompiledTemplate_free(PyObject *self)
{
    NTPY_CompiledTemplate *op = (NTPY_CompiledTemplate *)self;
    NT_VM_free(op->vm);
    op->vm = NULL;
    PyMem_Free(op);
}

PyObject *NTPY_CompiledTemplate_render(PyObject *self, PyObject *data)
{
    NTPY_CompiledTemplate *op = (NTPY_CompiledTemplate *)self;

    if (NT_VM_run(op->vm, data) < 0)
    {
        return NULL;
    }

    return NT_VM_join(op->vm);
}

static PyMethodDef CompiledTemplate_methods[] = {
    {"render", NTPY_CompiledTemplate_render, METH_O, "Render the template"},
    {NULL, NULL, 0, NULL}};

static PyType_Slot CompiledTemplate_slots[] = {
    {Py_tp_doc, "Compiled template"},
    {Py_tp_free, (void *)NTPY_CompiledTemplate_free},
    {Py_tp_methods, CompiledTemplate_methods},
    {0, NULL}};

static PyType_Spec CompiledTemplate_spec = {
    .name = "nano_template.CompiledTemplate",
    .basicsize = sizeof(NTPY_CompiledTemplate),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = CompiledTemplate_slots,
};

int nt_register_compiled_template_type(PyObject *module)
{
    PyObject *type_obj = PyType_FromSpec(&CompiledTemplate_spec);
    if (!type_obj)
    {
        return -1;
    }

    CompiledTemplate_TypeObject = (PyTypeObject *)type_obj;

    if (PyModule_AddObject(module, "CompiledTemplate", type_obj) < 0)
    {
        Py_DECREF(type_obj);
        CompiledTemplate_TypeObject = NULL;
        return -1;
    }

    return 0;
}