#ifndef ML_UNESCAPE_H
#define ML_UNESCAPE_H

#include "micro_liquid/common.h"

typedef enum
{
    QUOTE_SINGLE = 1,
    QUOTE_DOUBLE,
} QuoteKind;

/// @brief Replace `\uXXXX` escape sequences in `str` with their equivalent
/// Unicode code points.
/// @return A new reference to the unescaped string.
PyObject *unescape(PyObject *str, QuoteKind quote);

#endif
