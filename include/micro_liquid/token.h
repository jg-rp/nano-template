#ifndef ML_SPAN_H
#define ML_SPAN_H

#include "micro_liquid/common.h"

typedef enum
{
    TOK_WC_NONE = 1,
    TOK_WC_HYPHEN,
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
    TOK_EOF,
} ML_TokenKind;

static const char *ML_TokenKind_names[] = {
    [TOK_WC_NONE] = "TOK_WC_NONE",
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

static inline const char *ML_TokenKind_str(ML_TokenKind kind)
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
    ML_Token *token = (ML_Token *)PyMem_Malloc(sizeof(ML_Token));

    if (!token)
    {
        PyErr_NoMemory();
        return NULL;
    }

    token->start = start;
    token->end = end;
    token->kind = kind;
    return token;
}

// TODO: ML_Token_destroy for symmetry

// Assumes number of tokens is less than 32 or 64.
typedef Py_ssize_t ML_TokenMask;

static inline bool ML_Token_mask_test(ML_TokenKind kind, ML_TokenMask mask)
{
    return (mask & ((Py_ssize_t)1 << kind)) != 0;
}

#endif
