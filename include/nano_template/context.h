// SPDX-License-Identifier: MIT

#ifndef NT_CONTEXT_H
#define NT_CONTEXT_H

#include "nano_template/common.h"
#include "nano_template/token.h"

/// @brief Internal render context.
/// NT_Context_free decrements.
typedef struct NT_Context
{
    PyObject *str;

    PyObject **scope;
    Py_ssize_t size;
    Py_ssize_t capacity;

    PyObject *serializer;
    PyObject *undefined;
} NT_Context;

/// @brief Allocate and initialize a new NT_Context.
/// Increment reference counts for `str`, `globals`, `serializer` and
/// `undefined`. All are DECREFed in NT_Context_free.
/// @return Newly allocated NT_Context*, or NULL on memory error.
NT_Context *NT_Context_new(PyObject *str, PyObject *globals,
                           PyObject *serializer, PyObject *undefined);

void NT_Context_free(NT_Context *self);

/// @brief Lookup `key` in the current scope.
/// @return 0 if out was set to a new reference, or 1 if `key` is not in scope
/// or `key` is not a Python str.
int NT_Context_get(NT_Context *self, PyObject *key, PyObject **out);

/// @brief Extend scope with mapping `namespace`.
/// A reference to `namespace` is stolen and DECREFed in `NT_Context_free`.
/// @return 0 on success, -1 on failure.
int NT_Context_push(NT_Context *self, PyObject *namespace);

/// @brief Remove the namespace at the top of the scope stack.
/// Decrement the reference count for the popped namespace.
void NT_Context_pop(NT_Context *self);

#endif
