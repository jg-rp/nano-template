#include "micro_liquid/py_token_view.h"
#include "micro_liquid/py_tokenize.h"
#include <Python.h>

static PyMethodDef micro_liquid_methods[] = {
    {"tokenize", tokenize, METH_O,
     PyDoc_STR("tokenize(str) -> list[TokenView]")},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef micro_liquid_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_micro_liquid",
    .m_doc = "Minimal text templating.",
    .m_methods = micro_liquid_methods,
    .m_size = -1,
};

PyMODINIT_FUNC PyInit__micro_liquid(void)
{
    PyObject *mod = PyModule_Create(&micro_liquid_module);
    if (!mod)
        return NULL;

    if (register_token_view_type(mod) < 0)
    {
        Py_DECREF(mod);
        return NULL;
    }

    return mod;
}
