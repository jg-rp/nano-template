// SPDX-License-Identifier: MIT

#ifndef NT_UNESCAPE_H
#define NT_UNESCAPE_H

#include "nano_template/common.h"
#include "nano_template/token.h"

/// @brief Replace JSON-style escape sequences in the string represented by
/// `token` with their equivalent Unicode code points.
/// @return A new reference to the unescaped string.
PyObject *unescape(const NT_Token *token, PyObject *source);

#endif
