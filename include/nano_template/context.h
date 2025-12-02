// SPDX-License-Identifier: MIT

#ifndef NT_CONTEXT_H
#define NT_CONTEXT_H

#include "nano_template/common.h"
#include "nano_template/token.h"

typedef struct NT_RenderContext
{
    PyObject *str; // The input string

    PyObject **scope;    // A stack of dict[str, Any]
    Py_ssize_t size;     // Size of the stack
    Py_ssize_t capacity; // Stack capacity

    PyObject *serializer; // Callable[[object], str]
    PyObject *undefined;  // Type[Undefined]
} NT_RenderContext;

/// @brief Allocate and initialize a new NT_RenderContext.
/// Increment reference counts for `str`, `globals`, `serializer` and
/// `undefined`. All are DECREFed in NT_RenderContext_free.
/// @return Newly allocated NT_RenderContext*, or NULL on memory error.
NT_RenderContext *NT_RenderContext_new(PyObject *str, PyObject *globals,
                                       PyObject *serializer,
                                       PyObject *undefined);

void NT_RenderContext_free(NT_RenderContext *ctx);

/// @brief Lookup `key` in the current scope.
/// @return 0 if out was set to a new reference, or 1 if `key` is not in scope
/// or `key` is not a Python str.
int NT_RenderContext_get(const NT_RenderContext *ctx, PyObject *key,
                         PyObject **out);

/// @brief Extend scope with mapping `namespace`.
/// A reference to `namespace` is stolen and DECREFed in
/// `NT_RenderContext_free`.
/// @return 0 on success, -1 on failure.
int NT_RenderContext_push(NT_RenderContext *ctx, PyObject *namespace);

/// @brief Remove the namespace at the top of the scope stack.
/// Decrement the reference count for the popped namespace.
void NT_RenderContext_pop(NT_RenderContext *ctx);

#endif
