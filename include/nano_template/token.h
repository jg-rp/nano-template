// SPDX-License-Identifier: MIT

#ifndef NT_TOKEN_H
#define NT_TOKEN_H

#include "nano_template/allocator.h"
#include "nano_template/common.h"

// NOTE: Enum item order must match those in py/nano_template/_token_kind.py.
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
} NT_TokenKind;

// TODO: Make these names pretty? They are used in parser exception messages.
static const char *NT_TokenKind_names[] = {
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
    [TOK_EOF] = "TOK_EOF"};

/// @brief Return a string representation of `kind`.
static inline const char *NT_TokenKind_str(NT_TokenKind kind)
{
    if (kind >= TOK_WC_HYPHEN && kind <= TOK_EOF)
    {
        return NT_TokenKind_names[kind];
    }
    return "TOK_UNKNOWN";
}

/// @brief A start and end index into a Python string.
typedef struct NT_Token
{
    Py_ssize_t start;
    Py_ssize_t end;
    NT_TokenKind kind;
} NT_Token;

static inline NT_Token NT_Token_make(Py_ssize_t start, Py_ssize_t end,
                                     NT_TokenKind kind)
{
    NT_Token token = {start, end, kind};
    return token;
}

/// @brief Make a copy of `token`.
/// @return A pointer to the copy token owned by `mem`, or NULL of failure with
/// an exception set.
static inline NT_Token *NT_Token_copy(NT_Mem *mem, const NT_Token *token)
{
    NT_Token *new_token = NT_Mem_alloc(mem, sizeof(NT_Token));
    if (!new_token)
    {
        return NULL;
    }

    new_token->start = token->start;
    new_token->end = token->end;
    new_token->kind = token->kind;
    return new_token;
}

#endif
