#ifndef ML_UNESCAPE_H
#define ML_UNESCAPE_H

#include "micro_liquid/common.h"
#include "micro_liquid/token.h"

/// @brief Replace `\uXXXX` escape sequences in the string represented by token
/// with their equivalent Unicode code points.
/// @return A new reference to the unescaped string.
PyObject *unescape(ML_Token *token, PyObject *source);

#endif
