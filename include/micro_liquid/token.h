#ifndef ML_SPAN_H
#define ML_SPAN_H

#include "micro_liquid/common.h"

typedef enum
{
    TOK_WC_HYPHEN = 1,
    TOK_WC_TILDE,
    TOK_OUT_START,
    TOK_TAG_START,
    TOK_OUT_END,
    TOK_TAG_END,
    TOK_INT,
    TOK_SINGLE_QUOTE_STRING,
    TOK_DOUBLE_QUOTE_STRING,
    TOK_SINGLE_ESC_STRING,
    TOK_DOUBLE_ESC_STRING,
    TOK_WORD,
    TOK_IF_TAG,
    TOK_ELIF_TAG,
    TOK_ELSE_TAG,
    TOK_ENDIF_TAG,
    TOK_FOR_TAG,
    TOK_ENDFOR_TAG,
    TOK_OTHER,
    TOK_L_BRACKET,
    TOK_R_BRACKET,
    TOK_DOT,
    TOK_L_PAREN,
    TOK_R_PAREN,
    TOK_AND,
    TOK_OR,
    TOK_NOT,
    TOK_IN,
    TOK_ERROR,
    TOK_UNKNOWN,
    TOK_EOF
} ML_TokenKind;

static const char *ML_TokenKind_names[] = {
    [TOK_WC_HYPHEN] = "TOK_WC_HYPHEN",
    [TOK_WC_TILDE] = "TOK_WC_TILDE",
    [TOK_OUT_START] = "TOK_OUT_START",
    [TOK_TAG_START] = "TOK_TAG_START",
    [TOK_OUT_END] = "TOK_OUT_END",
    [TOK_TAG_END] = "TOK_TAG_END",
    [TOK_INT] = "TOK_INT",
    [TOK_SINGLE_QUOTE_STRING] = "TOK_SINGLE_QUOTE_STRING",
    [TOK_DOUBLE_QUOTE_STRING] = "TOK_DOUBLE_QUOTE_STRING",
    [TOK_SINGLE_ESC_STRING] = "TOK_SINGLE_ESC_STRING",
    [TOK_DOUBLE_ESC_STRING] = "TOK_DOUBLE_ESC_STRING",
    [TOK_WORD] = "TOK_WORD",
    [TOK_IF_TAG] = "TOK_IF_TAG",
    [TOK_ELIF_TAG] = "TOK_ELIF_TAG",
    [TOK_ELSE_TAG] = "TOK_ELSE_TAG",
    [TOK_ENDIF_TAG] = "TOK_ENDIF_TAG",
    [TOK_FOR_TAG] = "TOK_FOR_TAG",
    [TOK_ENDFOR_TAG] = "TOK_ENDFOR_TAG",
    [TOK_OTHER] = "TOK_OTHER",
    [TOK_L_BRACKET] = "TOK_L_BRACKET",
    [TOK_R_BRACKET] = "TOK_R_BRACKET",
    [TOK_DOT] = "TOK_DOT",
    [TOK_L_PAREN] = "TOK_L_PAREN",
    [TOK_R_PAREN] = "TOK_R_PAREN",
    [TOK_AND] = "TOK_AND",
    [TOK_OR] = "TOK_OR",
    [TOK_NOT] = "TOK_NOT",
    [TOK_IN] = "TOK_IN",
    [TOK_ERROR] = "TOK_ERROR",
    [TOK_UNKNOWN] = "TOK_UNKNOWN",
    [TOK_EOF] = "TOK_EOF"};

static inline const char *ML_TokenKind_to_string(ML_TokenKind kind)
{
    if (kind >= TOK_WC_HYPHEN && kind <= TOK_EOF)
        return ML_TokenKind_names[kind];
    return "TOK_UNKNOWN";
}

typedef struct ML_Token
{
    Py_ssize_t start;
    Py_ssize_t end;
    ML_TokenKind kind;
} ML_Token;

static inline ML_Token *ML_Token_new(Py_ssize_t start, Py_ssize_t end,
                                     ML_TokenKind kind)
{
    ML_Token *token = PyMem_Malloc(sizeof(ML_Token));
    if (!token)
        return NULL;

    token->start = start;
    token->end = end;
    token->kind = kind;
    return token;
}

static PyObject *ML_Token_text(ML_Token *span, PyObject *str)
{
    return PyUnicode_Substring(str, span->start, span->end);
}

#endif
