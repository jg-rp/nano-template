#include "micro_liquid/lexer.h"

static ML_Token *ML_Lexer_lex_markup(ML_Lexer *self);
static ML_Token *ML_Lexer_lex_expr(ML_Lexer *self);
static ML_Token *ML_Lexer_lex_tag(ML_Lexer *self);
static ML_Token *ML_Lexer_lex_other(ML_Lexer *self);
static ML_Token *ML_Lexer_lex_string(ML_Lexer *self, Py_UCS4 quote);
static ML_Token *ML_Lexer_lex_whitespace_control(ML_Lexer *self);
static ML_Token *ML_Lexer_lex_end_of_expr(ML_Lexer *self);

static inline Py_UCS4 ML_Lexer_peek(ML_Lexer *self);

static inline bool ML_Lexer_accept_while(PyObject *str, Py_ssize_t *pos,
                                         bool (*pred)(Py_UCS4));

static inline bool ML_Lexer_accept_until(PyObject *str, Py_ssize_t *pos,
                                         bool (*pred)(Py_UCS4));

static inline bool ML_Lexer_accept_until_ch(PyObject *str, Py_ssize_t *pos,
                                            char ch);

static inline bool ML_Lexer_accept_ch(PyObject *str, Py_ssize_t *pos, char ch);

static inline bool ML_Lexer_accept_str(PyObject *str, Py_ssize_t *pos,
                                       const char *sstr);

static inline bool ML_Lexer_accept_until_delim(PyObject *str, Py_ssize_t *pos);

static inline bool ML_Lexer_accept_keyword(PyObject *str, Py_ssize_t *pos,
                                           const char *word,
                                           Py_ssize_t word_length);

static inline bool is_ascii_digit(Py_UCS4 ch);
static inline bool is_ascii_alpha(Py_UCS4 ch);
static inline bool is_space_char(Py_UCS4 ch);
static inline bool is_whitespace_control(Py_UCS4 ch);
static inline bool is_word_boundary(Py_UCS4 ch);
static inline bool is_word_char_first(Py_UCS4 ch);
static inline bool is_word_char(Py_UCS4 ch);

ML_Lexer *ML_Lexer_new(PyObject *str)
{
    ML_Lexer *lexer = PyMem_Malloc(sizeof(ML_Lexer));
    if (!lexer)
        return NULL;

    Py_INCREF(str);
    lexer->str = str;
    lexer->pos = 0;
    ML_StateStack_init(&lexer->state);
    ML_StateStack_push(&lexer->state, STATE_MARKUP);
    return lexer;
}

void ML_Lexer_delete(ML_Lexer *self)
{
    Py_XDECREF(self->str);
    ML_StateStack_clear(&self->state);
    PyMem_Free(self);
}

ML_Token *ML_Lexer_next(ML_Lexer *self)
{
    PyObject *str = self->str;
    Py_ssize_t length = PyUnicode_GetLength(str);
    if (length < 0)
        return NULL;

    if (self->pos >= length)
        return ML_Token_new(length, length, TOK_EOF);

    // TODO: branchless jump table?

    switch (ML_StateStack_pop(&self->state))
    {
    case STATE_MARKUP:
        return ML_Lexer_lex_markup(self);
    case STATE_TAG:
        return ML_Lexer_lex_tag(self);
    case STATE_EXPR:
        return ML_Lexer_lex_expr(self);
    case STATE_OTHER:
        return ML_Lexer_lex_other(self);
    case STATE_WC:
        return ML_Lexer_lex_whitespace_control(self);
    default:
        PyErr_SetString(PyExc_ValueError, "unknown lexer state");
        return NULL;
    }
}

ML_Token **ML_Lexer_scan(ML_Lexer *self, Py_ssize_t *out_token_count)
{
    Py_ssize_t capacity = 128;
    Py_ssize_t token_count = 0;

    ML_Token **tokens = PyMem_Malloc(capacity * sizeof(ML_Token));
    if (!tokens)
        return NULL;

    for (;;)
    {
        ML_Token *tok = ML_Lexer_next(self);
        if (!tok)
        {
            PyMem_Free(tokens);
            return NULL;
        }

        if (token_count >= capacity)
        {
            capacity *= 2;
            ML_Token **tmp = PyMem_Realloc(tokens, capacity * sizeof(ML_Token));
            if (!tmp)
            {
                PyMem_Free(tokens);
                return NULL;
            }
            tokens = tmp;
        }

        tokens[token_count++] = tok;
        if (tok->kind == TOK_EOF)
            break;
    }

    *out_token_count = token_count;
    return tokens;
}

static ML_Token *ML_Lexer_lex_markup(ML_Lexer *self)
{
    Py_ssize_t start = self->pos;

    if (ML_Lexer_accept_str(self->str, &self->pos, "{{"))
    {
        ML_StateStack_push(&self->state, STATE_EXPR);

        if (is_whitespace_control(ML_Lexer_peek(self)))
            ML_StateStack_push(&self->state, STATE_WC);

        return ML_Token_new(start, self->pos, TOK_OUT_START);
    }

    if (ML_Lexer_accept_str(self->str, &self->pos, "{%"))
    {
        ML_StateStack_push(&self->state, STATE_TAG);

        if (is_whitespace_control(ML_Lexer_peek(self)))
            ML_StateStack_push(&self->state, STATE_WC);

        return ML_Token_new(start, self->pos, TOK_TAG_START);
    }

    return ML_Lexer_lex_other(self);
}

static ML_Token *ML_Lexer_lex_tag(ML_Lexer *self)
{
    ML_StateStack_push(&self->state, STATE_EXPR);
    ML_Lexer_accept_while(self->str, &self->pos, is_space_char);
    Py_ssize_t start = self->pos;

    if (ML_Lexer_accept_keyword(self->str, &self->pos, "if", 2))
    {
        return ML_Token_new(start, self->pos, TOK_IF_TAG);
    }

    if (ML_Lexer_accept_keyword(self->str, &self->pos, "elif", 4))
    {
        return ML_Token_new(start, self->pos, TOK_ELIF_TAG);
    }

    if (ML_Lexer_accept_keyword(self->str, &self->pos, "else", 4))
    {
        return ML_Token_new(start, self->pos, TOK_ELSE_TAG);
    }

    if (ML_Lexer_accept_keyword(self->str, &self->pos, "endif", 5))
    {
        return ML_Token_new(start, self->pos, TOK_ENDIF_TAG);
    }

    if (ML_Lexer_accept_keyword(self->str, &self->pos, "for", 3))
    {
        return ML_Token_new(start, self->pos, TOK_FOR_TAG);
    }

    if (ML_Lexer_accept_keyword(self->str, &self->pos, "endfor", 6))
    {
        return ML_Token_new(start, self->pos, TOK_ENDFOR_TAG);
    }

    return ML_Token_new(start, start, TOK_ERROR);
}

static ML_Token *ML_Lexer_lex_expr(ML_Lexer *self)
{
    ML_Lexer_accept_while(self->str, &self->pos, is_space_char);
    Py_ssize_t start = self->pos;

    Py_UCS4 ch = ML_Lexer_peek(self);
    ML_StateStack_push(&self->state, STATE_EXPR);

    switch (ch)
    {
    case '"':
        self->pos++;
        return ML_Lexer_lex_string(self, '"');
    case '\'':
        self->pos++;
        return ML_Lexer_lex_string(self, '\'');
    case '[':
        self->pos++;
        return ML_Token_new(start, self->pos, TOK_L_BRACKET);
    case ']':
        self->pos++;
        return ML_Token_new(start, self->pos, TOK_R_BRACKET);
    case '(':
        self->pos++;
        return ML_Token_new(start, self->pos, TOK_L_PAREN);
    case ')':
        self->pos++;
        return ML_Token_new(start, self->pos, TOK_R_PAREN);
    case '-':
        self->pos++;
        if (!ML_Lexer_accept_while(self->str, &self->pos, is_ascii_digit))
        {
            return ML_Token_new(start, self->pos, TOK_WC_HYPHEN);
        }
        // Negative integer
        return ML_Token_new(start, self->pos, TOK_INT);
    case '~':
        self->pos++;
        return ML_Token_new(start, self->pos, TOK_WC_TILDE);
    }

    if (ML_Lexer_accept_while(self->str, &self->pos, is_ascii_digit))
    {
        return ML_Token_new(start, self->pos, TOK_INT);
    }

    if (ML_Lexer_accept_keyword(self->str, &self->pos, "and", 3))
    {
        return ML_Token_new(start, self->pos, TOK_AND);
    }

    if (ML_Lexer_accept_keyword(self->str, &self->pos, "or", 2))
    {
        return ML_Token_new(start, self->pos, TOK_OR);
    }

    if (ML_Lexer_accept_keyword(self->str, &self->pos, "not", 3))
    {
        return ML_Token_new(start, self->pos, TOK_NOT);
    }

    if (ML_Lexer_accept_keyword(self->str, &self->pos, "in", 2))
    {
        return ML_Token_new(start, self->pos, TOK_IN);
    }

    if (is_word_char_first(ch))
    {
        self->pos++;
        ML_Lexer_accept_while(self->str, &self->pos, is_word_char);
        return ML_Token_new(start, self->pos, TOK_WORD);
    }

    return ML_Lexer_lex_end_of_expr(self);
}

static ML_Token *ML_Lexer_lex_whitespace_control(ML_Lexer *self)
{
    Py_ssize_t start = self->pos;

    if (ML_Lexer_accept_ch(self->str, &self->pos, '-'))
    {
        return ML_Token_new(start, self->pos, TOK_WC_HYPHEN);
    }

    if (ML_Lexer_accept_ch(self->str, &self->pos, '~'))
    {
        return ML_Token_new(start, self->pos, TOK_WC_TILDE);
    }

    return NULL; // unreachable
}

static ML_Token *ML_Lexer_lex_other(ML_Lexer *self)
{
    Py_ssize_t start = self->pos;

    if (ML_Lexer_accept_until_delim(self->str, &self->pos))
    {
        return ML_Token_new(start, self->pos, TOK_OTHER);
    }

    // Output extends to the end of the input string.

    Py_ssize_t length = PyUnicode_GetLength(self->str);
    if (length < 0)
        return NULL;

    self->pos = length;
    return ML_Token_new(start, self->pos, TOK_OTHER);
}

static ML_Token *ML_Lexer_lex_end_of_expr(ML_Lexer *self)
{
    Py_ssize_t start = self->pos;

    if (ML_Lexer_accept_str(self->str, &self->pos, "%}"))
    {
        ML_StateStack_pop(&self->state);
        return ML_Token_new(start, self->pos, TOK_TAG_END);
    }

    if (ML_Lexer_accept_str(self->str, &self->pos, "}}"))
    {
        ML_StateStack_pop(&self->state);
        return ML_Token_new(start, self->pos, TOK_OUT_END);
    }

    self->pos++;
    return ML_Token_new(start, self->pos, TOK_UNKNOWN);
}

static ML_Token *ML_Lexer_lex_string(ML_Lexer *self, Py_UCS4 quote)
{
    Py_ssize_t start = self->pos;
    Py_UCS4 ch = PyUnicode_ReadChar(self->str, self->pos);
    ML_TokenKind kind =
        quote == '\'' ? TOK_SINGLE_QUOTE_STRING : TOK_DOUBLE_QUOTE_STRING;

    if (ch == quote)
    {
        // Empty string
        self->pos++;
        return ML_Token_new(start, start, kind);
    }

    for (;;)
    {
        ch = PyUnicode_ReadChar(self->str, self->pos);

        if (ch == '\\')
        {
            self->pos++;
            kind =
                quote == '\'' ? TOK_SINGLE_ESC_STRING : TOK_DOUBLE_ESC_STRING;
        }
        else if (ch == quote)
        {
            self->pos++;
            return ML_Token_new(start, self->pos - 1, kind);
        }
        else if (ch == (Py_UCS4)-1)
        {
            // end of input
            // unclosed string literal
            return ML_Token_new(start, self->pos, TOK_ERROR);
        }

        self->pos++;
    }

    return ML_Token_new(start, self->pos, TOK_ERROR);
}

static inline Py_UCS4 ML_Lexer_peek(ML_Lexer *self)
{
    return PyUnicode_ReadChar(self->str, self->pos);
}

static inline bool ML_Lexer_accept_while(PyObject *str, Py_ssize_t *pos,
                                         bool (*pred)(Py_UCS4))
{
    Py_ssize_t start = *pos;
    Py_ssize_t length = PyUnicode_GetLength(str);

    while (*pos < length)
    {
        Py_UCS4 ch = PyUnicode_ReadChar(str, *pos);
        if (!pred(ch))
            break;
        (*pos)++;
    }

    return *pos > start;
}

static inline bool ML_Lexer_accept_until(PyObject *str, Py_ssize_t *pos,
                                         bool (*pred)(Py_UCS4))
{
    Py_ssize_t start = *pos;
    Py_ssize_t length = PyUnicode_GetLength(str);

    while (*pos < length)
    {
        Py_UCS4 ch = PyUnicode_ReadChar(str, *pos);
        if (pred(ch))
            break;
        (*pos)++;
    }

    return *pos > start;
}

static inline bool ML_Lexer_accept_until_ch(PyObject *str, Py_ssize_t *pos,
                                            char ch)
{
    Py_ssize_t start = *pos;
    Py_ssize_t length = PyUnicode_GetLength(str);

    while (*pos < length)
    {
        if (PyUnicode_ReadChar(str, *pos) != (unsigned char)ch)
            break;
        (*pos)++;
    }

    return *pos > start;
}

static inline bool ML_Lexer_accept_ch(PyObject *str, Py_ssize_t *pos, char ch)
{
    Py_ssize_t length = PyUnicode_GetLength(str);

    if (*pos >= length)
        return false;

    if (PyUnicode_ReadChar(str, *pos) != (unsigned char)ch)
        return false;

    (*pos)++;
    return true;
}

static inline bool ML_Lexer_accept_str(PyObject *str, Py_ssize_t *pos,
                                       const char *sstr)
{
    Py_ssize_t start = *pos;
    Py_ssize_t length = PyUnicode_GetLength(str);
    Py_ssize_t i = 0;

    while (sstr[i] && start + i < length)
    {
        Py_UCS4 ch = PyUnicode_ReadChar(str, start + i);

        // ASCII-only comparison
        if ((unsigned char)ch != (unsigned char)sstr[i])
            return false;

        i++;
    }

    if (sstr[i] != '\0')
        return false; // string didn't fully match

    *pos = start + i; // advance the position on success
    return true;
}

static inline bool ML_Lexer_accept_keyword(PyObject *str, Py_ssize_t *pos,
                                           const char *word,
                                           Py_ssize_t word_length)
{
    Py_ssize_t start = *pos;
    Py_ssize_t length = PyUnicode_GetLength(str);

    // not enough room for keyword
    if (start + word_length > length)
        return false;

    // check characters one by one
    for (Py_ssize_t i = 0; i < word_length; i++)
    {
        Py_UCS4 ch = PyUnicode_ReadChar(str, start + i);
        if (ch != (unsigned char)word[i])
            return false;
    }

    // check the boundary after the keyword
    Py_UCS4 next = 0;
    if (start + word_length < length)
        next = PyUnicode_ReadChar(str, start + word_length);

    if (!is_word_boundary(next))
        return false;

    *pos += word_length;
    return true;
}

static inline bool ML_Lexer_accept_until_delim(PyObject *str, Py_ssize_t *pos)
{
    Py_ssize_t start = *pos;
    Py_ssize_t length = PyUnicode_GetLength(str);

    while (*pos < length)
    {
        Py_ssize_t found = PyUnicode_FindChar(str, '{', *pos, length, 1);
        if (found == -1 || found + 1 >= length)
        {
            *pos = length;
            break;
        }

        Py_UCS4 next = PyUnicode_ReadChar(str, found + 1);
        if (next == '{' || next == '%')
        {
            *pos = found;
            break;
        }

        *pos = found + 1;
    }

    return *pos > start;
}

static inline bool is_ascii_digit(Py_UCS4 ch)
{
    return (ch >= '0' && ch <= '9');
}

static inline bool is_ascii_alpha(Py_UCS4 ch)
{
    return ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'));
}

static inline bool is_space_char(Py_UCS4 ch)
{
    return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
}

static inline bool is_whitespace_control(Py_UCS4 ch)
{
    return (ch == '-' || ch == '~');
}

static inline bool is_word_boundary(Py_UCS4 ch)
{
    return ch == 0 || // end of string
           ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '[' ||
           ch == ']' || ch == '(' || ch == ')' || ch == '.' || ch == '%' ||
           ch == '}' || ch == '-' || ch == '\'' || ch == '"';
}

static inline bool is_word_char_first(Py_UCS4 ch)
{
    return ((ch >= 0x80 && ch <= 0xFFFF) || (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') || ch == '_');
}

static inline bool is_word_char(Py_UCS4 ch)
{
    return ((ch >= 0x80 && ch <= 0xFFFF) || (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' ||
            ch == '-');
}
