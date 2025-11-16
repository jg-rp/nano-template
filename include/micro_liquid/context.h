#ifndef ML_CONTEXT_H
#define ML_CONTEXT_H

#include "micro_liquid/common.h"
#include "micro_liquid/token.h"

// TODO: consistent doc comment syntax

typedef struct ML_Context
{
    PyObject *str;    // Source string
    PyObject **scope; // **Mapping[str, object]
    Py_ssize_t size;
    Py_ssize_t capacity;
    PyObject *serializer; // Callable[[object], str]
    PyObject *undefined;  // Type[Undefined]
} ML_Context;

ML_Context *ML_Context_new(PyObject *str, PyObject *globals,
                           PyObject *serializer, PyObject *undefined);

void ML_Context_destroy(ML_Context *self);

/**
 * Lookup `key` in the current scope. Return NULL if `key` is not in scope
 * or `key` is not a Python str.
 */
PyObject *ML_Context_get(ML_Context *self, PyObject *key, ML_Token *token);

/**
 * Extend the current scope with `namespace`.
 *
 * Steal a reference to `namespace` and DECREF it in `ML_Context_destroy`.
 *
 * @param namespace // Mapping[str, object]
 * @return 0 on success, -1 on failure.
 */
Py_ssize_t ML_Context_push(ML_Context *self, PyObject *namespace);

/**
 * Remove the namespace from the top of the scope stack and free that namespace.
 */
PyObject *ML_Context_pop(ML_Context *self);

#endif
