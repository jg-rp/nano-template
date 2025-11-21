#ifndef ML_OBJECT_LIST_H
#define ML_OBJECT_LIST_H

#include "micro_liquid/common.h"

// TODO: doc comments

typedef struct ML_ObjList
{
    PyObject **items;
    Py_ssize_t size;
    Py_ssize_t capacity;
} ML_ObjList;

// Helpers for building arrays of PyObject*.
ML_ObjList *ML_ObjList_new(void);
void ML_ObjList_dealloc(ML_ObjList *self);
void ML_ObjList_disown(ML_ObjList *self);
Py_ssize_t ML_ObjList_grow(ML_ObjList *self);

Py_ssize_t ML_ObjList_append(ML_ObjList *self, PyObject *obj);

/// @brief Join the Unicode items in the list into a single string.
/// @return A new PyUnicode object on success, or NULL on error.
PyObject *ML_ObjList_join(ML_ObjList *self);

#endif