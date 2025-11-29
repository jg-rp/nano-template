#include "micro_liquid/unescape.h"
#include "micro_liquid/error.h"
#include "micro_liquid/string_buffer.h"
#include "micro_liquid/token.h"

/// @brief Parse hex digits in `str` starting at position `pos`.
/// @return A code point, or -1 on failure with an exception set.
static inline int code_point_from_digits(PyObject *str, Py_ssize_t *pos,
                                         ML_Token *token);

/// Return true is `code_point` is a high surrogate, false otherwise.
static inline bool is_high_surrogate(int code_point);

/// Return true is `code_point` is a low surrogate, false otherwise.
static inline bool is_low_surrogate(int code_point);

/// @brief Decode `XXXX` or `XXXX\uXXXX` sequence in `str` starting at
/// position `pos`.
/// @return A new Unicode object, or NULL on failure with an exception set.
static PyObject *decode_unicode_escape(PyObject *str, Py_ssize_t *pos,
                                       Py_ssize_t length, ML_Token *token);

/// @brief Decode a `\X`, `\uXXXX` or `\uXXXX\uXXXX` sequence in `str` starting
/// at position `pos`.
/// @return A new Unicode object, or NULL on failure with an exception set.
static PyObject *decode_escape(PyObject *str, Py_ssize_t *pos,
                               Py_ssize_t length, ML_Token *token);

PyObject *unescape(ML_Token *token, PyObject *source)
{
    PyObject *result = NULL;
    PyObject *buf = NULL;
    PyObject *str = NULL;
    PyObject *substring = NULL;

    str = PyUnicode_Substring(source, token->start, token->end);
    if (!str)
    {
        return NULL;
    }

    Py_ssize_t length = PyUnicode_GetLength(str);
    if (!length)
    {
        goto cleanup;
    }

    buf = StringBuffer_new();
    if (!buf)
    {
        goto cleanup;
    }

    Py_ssize_t pos = 0;
    while (pos < length)
    {
        Py_ssize_t index = PyUnicode_FindChar(str, '\\', pos, length, 1);

        if (index < 0)
        {
            // End of string, no more escape sequences.
            substring = PyUnicode_Substring(str, pos, length);
            if (!substring)
            {
                goto cleanup;
            }

            if (StringBuffer_append(buf, substring) < 0)
            {
                goto cleanup;
            }

            Py_DECREF(substring);
            substring = NULL;
            break;
        }

        if (index > pos)
        {
            // Buffer substring before the escape sequence.
            substring = PyUnicode_Substring(str, pos, index);
            if (!substring)
            {
                goto cleanup;
            }

            if (StringBuffer_append(buf, substring) < 0)
            {
                goto cleanup;
            }

            Py_DECREF(substring);
            substring = NULL;
        }

        substring = decode_escape(str, &pos, length, token);
        if (!substring)
        {
            goto cleanup;
        }

        if (StringBuffer_append(buf, substring) < 0)
        {
            goto cleanup;
        }

        Py_DECREF(substring);
        substring = NULL;
    }

    result = StringBuffer_finish(buf);
    buf = NULL;
    // Fall through

cleanup:
    Py_XDECREF(str);
    Py_XDECREF(substring);
    Py_XDECREF(buf);
    return result;
}

static PyObject *decode_escape(PyObject *str, Py_ssize_t *pos,
                               Py_ssize_t length, ML_Token *token)
{
    (*pos)++; // Move past `\`
    if (*pos >= length)
    {
        parser_error(token, "invalid escape sequence");
        return NULL;
    }

    Py_UCS4 ch = PyUnicode_ReadChar(str, *pos);
    if (ch == (Py_UCS4)-1)
    {
        return NULL;
    }

    (*pos)++;

    switch (ch)
    {
    case '"':
        if (token->kind == TOK_SINGLE_ESC_STRING)
        {
            parser_error(token, "invalid '\\\"' escape sequence");
            return NULL;
        }
        return PyUnicode_FromOrdinal('"');
    case '\'':
        if (token->kind == TOK_DOUBLE_ESC_STRING)
        {
            parser_error(token, "invalid '\\'' escape sequence");
            return NULL;
        }
        return PyUnicode_FromOrdinal('\'');
    case '\\':
        return PyUnicode_FromOrdinal('\\');
    case '/':
        return PyUnicode_FromOrdinal('/');
    case 'b':
        return PyUnicode_FromOrdinal('\b');
    case 'f':
        return PyUnicode_FromOrdinal('\f');
    case 'n':
        return PyUnicode_FromOrdinal('\n');
    case 'r':
        return PyUnicode_FromOrdinal('\r');
    case 't':
        return PyUnicode_FromOrdinal('\t');
    case 'u':
        return decode_unicode_escape(str, pos, length, token);
    default:
        parser_error(token, "unknown escape sequence '\\%c'", ch);
        return NULL;
    }
}

static PyObject *decode_unicode_escape(PyObject *str, Py_ssize_t *pos,
                                       Py_ssize_t length, ML_Token *token)
{
    if (*pos + 3 >= length)
    {
        parser_error(token, "incomplete escape sequence");
        return NULL;
    }

    Py_UCS4 code_point = code_point_from_digits(str, pos, token);
    if (code_point == (Py_UCS4)-1)
    {
        return NULL;
    }

    if (is_low_surrogate(code_point))
    {
        parser_error(token, "unexpected low surrogate");
        return NULL;
    }

    if (is_high_surrogate(code_point))
    {
        // Expect `\uXXXX`
        if (*pos + 5 >= length)
        {
            parser_error(token, "incomplete escape sequence");
            return NULL;
        }

        Py_UCS4 slash = PyUnicode_ReadChar(str, *pos);
        Py_UCS4 u = PyUnicode_ReadChar(str, *pos + 1);

        if (slash != '\\' || u != 'u')
        {
            parser_error(token, "expected low surrogate");
            return NULL;
        }

        (*pos) += 2;

        Py_UCS4 low_surrogate = code_point_from_digits(str, pos, token);
        if (low_surrogate == (Py_UCS4)-1)
        {
            return NULL;
        }

        if (!is_low_surrogate(low_surrogate))
        {
            parser_error(token, "expected low surrogate");
            return NULL;
        }

        code_point = 0x10000 + (((code_point & 0x03FF) << 10) |
                                (low_surrogate & 0x03FF));
    }

    return PyUnicode_FromOrdinal(code_point);
}

static inline bool is_high_surrogate(int code_point)
{
    return code_point >= 0xD800 && code_point <= 0xDBFF;
}

static inline bool is_low_surrogate(int code_point)
{
    return code_point >= 0xDC00 && code_point <= 0xDFFF;
}

static inline int code_point_from_digits(PyObject *str, Py_ssize_t *pos,
                                         ML_Token *token)
{
    int code_point = 0;

    for (Py_ssize_t i = 0; i < 4; i++)
    {
        Py_UCS4 digit = PyUnicode_ReadChar(str, *pos);
        if (digit == (Py_UCS4)-1)
        {
            return -1;
        }

        code_point <<= 4;

        switch (digit)
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            code_point |= (digit - '0');
            break;
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            code_point |= (digit - 'a' + 10);
            break;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            code_point |= (digit - 'A' + 10);
            break;
        default:
            parser_error(token, "invalid hex digit `%c` in escape sequence",
                         digit);
            return -1;
        }

        (*pos)++;
    }

    return code_point;
}