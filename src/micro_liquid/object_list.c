#include "micro_liquid/object_list.h"

ML_ObjList *ML_ObjList_new(void)
{
    ML_ObjList *list = PyMem_Malloc(sizeof(ML_ObjList));
    if (!list)
    {
        PyErr_NoMemory();
        return NULL;
    }

    list->size = 0;
    list->capacity = 4;
    list->items = PyMem_Malloc(sizeof(PyObject *) * list->capacity);
    if (!list->items)
    {
        PyErr_NoMemory();
        PyMem_Free(list);
        return NULL;
    }

    return list;
}

void ML_ObjList_dealloc(ML_ObjList *self)
{
    // XXX: fixme

    // for (Py_ssize_t i = 0; i < self->size; i++)
    // {
    //     if (self->items[i])
    //     {
    //         PySys_WriteStdout("!!: %ld of %ld\n", i, self->size);
    //         Py_XDECREF(self->items[i]);
    //     }
    // }

    PyMem_Free(self->items);
    PyMem_Free(self);
}

void ML_ObjList_disown(ML_ObjList *self)
{
    PyMem_Free(self);
}

Py_ssize_t ML_ObjList_grow(ML_ObjList *self)
{
    Py_ssize_t new_cap = (self->capacity == 0) ? 4 : (self->capacity * 2);

    PyObject **new_items =
        PyMem_Realloc(self->items, sizeof(PyObject *) * new_cap);

    if (!new_items)
    {
        PyErr_NoMemory();
        return -1;
    }

    self->items = new_items;
    self->capacity = new_cap;
    return 0;
}

Py_ssize_t ML_ObjList_append(ML_ObjList *self, PyObject *obj)
{
    if (self->size == self->capacity)
    {
        if (ML_ObjList_grow(self) != 0)
        {
            return -1;
        }
    }

    self->items[self->size++] = obj;
    return 0;
}

PyObject *ML_ObjList_join(ML_ObjList *self)
{
    if (!self)
    {
        return NULL;
    }

    Py_ssize_t n = (Py_ssize_t)self->size;
    PyObject *list = NULL;
    PyObject *sep = NULL;
    PyObject *joined = NULL;

    list = PyList_New(n);
    if (!list)
    {
        goto fail;
    }

    for (Py_ssize_t i = 0; i < n; i++)
    {
        PyObject *item = self->items[i];

        // TODO:
        // if (!PyUnicode_Check(item))
        // {
        //     PyErr_Format(PyExc_TypeError, "expected str, got %.200s",
        //                  Py_TYPE(item)->tp_name);
        //     goto fail;
        // }

        // PyList_SetItem steals a reference *if* it succeeds,
        // so we need to INCREF first.
        Py_INCREF(item);

        if (PyList_SetItem(list, i, item) < 0)
        {
            // Undo the INCREF if insertion failed.
            Py_DECREF(item);
            goto fail;
        }
    }

    sep = PyUnicode_FromString("");
    if (!sep)
    {
        goto fail;
    }

    joined = PyUnicode_Join(sep, list);
    if (!joined)
    {
        goto fail;
    }

    Py_DECREF(sep);
    Py_DECREF(list);
    return joined;

fail:
    Py_XDECREF(sep);
    Py_XDECREF(list);
    return NULL;
}
