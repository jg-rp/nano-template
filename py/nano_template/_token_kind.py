# SPDX-License-Identifier: MIT

from enum import IntEnum
from enum import auto


class TokenKind(IntEnum):
    TOK_WC_NONE = auto()
    TOK_WC_HYPHEN = auto()
    TOK_WC_TILDE = auto()
    TOK_OUT_START = auto()
    TOK_TAG_START = auto()
    TOK_OUT_END = auto()
    TOK_TAG_END = auto()
    TOK_INT = auto()
    TOK_SINGLE_QUOTE_STRING = auto()
    TOK_DOUBLE_QUOTE_STRING = auto()
    TOK_SINGLE_ESC_STRING = auto()
    TOK_DOUBLE_ESC_STRING = auto()
    TOK_WORD = auto()
    TOK_IF_TAG = auto()
    TOK_ELIF_TAG = auto()
    TOK_ELSE_TAG = auto()
    TOK_ENDIF_TAG = auto()
    TOK_FOR_TAG = auto()
    TOK_ENDFOR_TAG = auto()
    TOK_OTHER = auto()
    TOK_L_BRACKET = auto()
    TOK_R_BRACKET = auto()
    TOK_DOT = auto()
    TOK_L_PAREN = auto()
    TOK_R_PAREN = auto()
    TOK_AND = auto()
    TOK_OR = auto()
    TOK_NOT = auto()
    TOK_IN = auto()
    TOK_ERROR = auto()
    TOK_UNKNOWN = auto()
    TOK_EOF = auto()
