#ifndef ML_CONTEXT_H
#define ML_CONTEXT_H

#include "micro_liquid/common.h"

typedef struct ML_Context
{
    PyObject *scope;      // Mapping[str, object]
    PyObject *serializer; // Callable[[object], str]
    PyObject *undefined;  // Type[Undefined]
} ML_Context;

ML_Context *ML_Context_new(PyObject *scope, PyObject *serializer,
                           PyObject *undefined);

void ML_Context_destroy(ML_Context *self);

#endif
