#ifndef ML_CONTEXT_H
#define ML_CONTEXT_H

#include "micro_liquid/common.h"
#include "micro_liquid/token.h"

/// @brief Internal render context.
/// Reference counts for all PyObject* will be incremented by ML_Context_new;
/// ML_Context_dealloc decrements.
typedef struct ML_Context
{
    PyObject *str;
    PyObject **scope;
    Py_ssize_t size;
    Py_ssize_t capacity;
    PyObject *serializer;
    PyObject *undefined;
} ML_Context;

/// @brief Allocate and initialize a new ML_Context.
/// @return Newly allocated ML_Context*, or NULL on memory error.
ML_Context *ML_Context_new(PyObject *str, PyObject *globals,
                           PyObject *serializer, PyObject *undefined);

void ML_Context_dealloc(ML_Context *self);

/// @brief Lookup `key` in the current scope.
/// @return A new reference, or NULL if `key` is not in scope or `key` is not a
/// Python str.
PyObject *ML_Context_get(ML_Context *self, PyObject *key, ML_Token *token);

/// @brief Extend scope with mapping `namespace`.
/// A reference to `namespace` is stolen and DECREFed in `ML_Context_dealloc`.
/// @return 0 on success, -1 on failure.
Py_ssize_t ML_Context_push(ML_Context *self, PyObject *namespace);

/// @brief Remove the namespace at the top of the scope stack.
/// Decrement the reference count for the popped namespace.
void ML_Context_pop(ML_Context *self);

#endif
