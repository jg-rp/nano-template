// SPDX-License-Identifier: MIT

#include "nano_template/py_parse.h"
#include "nano_template/py_template.h"
#include "nano_template/py_token_view.h"
#include "nano_template/py_tokenize.h"
#include <Python.h>

static PyMethodDef nano_template_methods[] = {
    {"parse", (PyCFunction)parse, METH_VARARGS,
     PyDoc_STR("parse(str) -> Template")},
    {"tokenize", tokenize, METH_O,
     PyDoc_STR("tokenize(str) -> list[TokenView]")},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef nano_template_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_nano_template",
    .m_doc = "Minimal text templating.",
    .m_methods = nano_template_methods,
    .m_size = -1,
};

// TODO: Multi-Phase Initialization? Sticking with single-phase for now.

PyMODINIT_FUNC PyInit__nano_template(void)
{
    PyObject *mod = PyModule_Create(&nano_template_module);
    if (!mod)
    {
        return NULL;
    }

    if (nt_register_token_view_type(mod) < 0)
    {
        Py_DECREF(mod);
        return NULL;
    }

    if (nt_register_template_type(mod) < 0)
    {
        Py_DECREF(mod);
        return NULL;
    }

    return mod;
}
