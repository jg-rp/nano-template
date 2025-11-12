#ifndef ML_CONTEXT_H
#define ML_CONTEXT_H

#include "micro_liquid/common.h"

// TODO: change scope to be an array of PyObject*?

typedef struct ML_Context
{
    PyObject *scope;      // List[Mapping[str, object]]
    PyObject *serializer; // Callable[[object], str]
    PyObject *undefined;  // Type[Undefined]
} ML_Context;

ML_Context *ML_Context_new(PyObject *scope, PyObject *serializer,
                           PyObject *undefined);

void ML_Context_destroy(ML_Context *self);

/**
 * Lookup `key` in the current scope. Return NULL if `key` is not in scope
 * or `key` is not a Python str.
 */
PyObject *ML_Context_get(ML_Context *self, PyObject *key);

/**
 * Extend the current scope with `namespace`.
 *
 * The render context takes ownership of `namespace` and frees it in
 * `ML_Context_pop`.
 *
 * @param namespace // Mapping[str, object]
 */
void ML_Context_push(ML_Context *self, PyObject *namespace);

/**
 * Remove the namespace from the top of the scope stack and free that namespace.
 */
void ML_Context_pop(ML_Context *self);

#endif
