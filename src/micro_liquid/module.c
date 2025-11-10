#include <Python.h>

static struct PyModuleDef micro_liquid_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_micro_liquid",
    .m_doc = "Minimal text templating.",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit__micro_liquid(void)
{
    PyObject *mod = PyModule_Create(&micro_liquid_module);
    if (!mod)
        return NULL;

    // TODO: add objects to mod

    return mod;
}
