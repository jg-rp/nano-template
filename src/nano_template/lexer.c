// SPDX-License-Identifier: MIT

#include "nano_template/lexer.h"

/// @brief Push a new state onto the state stack.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Lexer_push(NT_Lexer *l, NT_State state);

/// @brief Remove the state at the top of the state stack.
/// @return The removed state or STATE_MARKUP if the stack is empty.
static NT_State NT_Lexer_pop(NT_Lexer *l);

/// Return the character at pos without advancing.
static inline Py_UCS4 NT_Lexer_read_char(NT_Lexer *l);

/// Return the character at position n without advancing.
static inline Py_UCS4 NT_Lexer_read_char_n(NT_Lexer *l, Py_ssize_t n);

/// @brief Advance pos while predicate pred is true.
/// @return true if at least one character was accepted, false otherwise.
static inline bool NT_Lexer_accept_while(NT_Lexer *l, bool (*pred)(Py_UCS4));

/// @brief Advance pos by one if the character at pos is equal to ch.
/// @return true if ch matched at pos, false otherwise.
static inline bool NT_Lexer_accept_ch(NT_Lexer *l, char ch);

/// @brief Advance pos by the length of sstr if sstr matches a pos.
/// @return true if sstr matched at pos, false otherwise.
static inline bool NT_Lexer_accept_str(NT_Lexer *l, const char *sstr);

/// @brief Keep advancing pos until we find an opening markup delimiter.
/// @return true if pos was updated, false if we reached end of input.
static inline bool NT_Lexer_accept_until_delim(NT_Lexer *l);

/// @brief Advance pos by word_length if word matches at pos as a whole word.
/// @return true if pos was update, false otherwise.
static inline bool NT_Lexer_accept_keyword(NT_Lexer *l, const char *word,
                                           Py_ssize_t word_length);

/// Lexer state handlers.
static NT_Token NT_Lexer_lex_markup(NT_Lexer *l);
static NT_Token NT_Lexer_lex_expr(NT_Lexer *l);
static NT_Token NT_Lexer_lex_tag(NT_Lexer *l);
static NT_Token NT_Lexer_lex_other(NT_Lexer *l);
static NT_Token NT_Lexer_lex_string(NT_Lexer *l, Py_UCS4 quote);
static NT_Token NT_Lexer_lex_whitespace_control(NT_Lexer *l);
static NT_Token NT_Lexer_lex_end_of_expr(NT_Lexer *l);

/// Predicates for NT_Lexer_accept_while.
static inline bool is_ascii_digit(Py_UCS4 ch);
static inline bool is_space_char(Py_UCS4 ch);
static inline bool is_whitespace_control(Py_UCS4 ch);
static inline bool is_word_boundary(Py_UCS4 ch);
static inline bool is_word_char_first(Py_UCS4 ch);
static inline bool is_word_char(Py_UCS4 ch);

typedef NT_Token (*LexFn)(NT_Lexer *l);

static LexFn state_table[] = {
    [STATE_MARKUP] = NT_Lexer_lex_markup,
    [STATE_TAG] = NT_Lexer_lex_tag,
    [STATE_EXPR] = NT_Lexer_lex_expr,
    [STATE_OTHER] = NT_Lexer_lex_other,
    [STATE_WC] = NT_Lexer_lex_whitespace_control,
};

NT_Lexer *NT_Lexer_new(PyObject *str)
{
    Py_ssize_t length = PyUnicode_GetLength(str);
    if (length < 0)
    {
        return NULL;
    }

    NT_Lexer *lexer = PyMem_Malloc(sizeof(NT_Lexer));
    if (!lexer)
    {
        PyErr_NoMemory();
        return NULL;
    }

    Py_INCREF(str);
    lexer->str = str;
    lexer->length = length;
    lexer->pos = 0;
    lexer->state = NULL;
    lexer->stack_capacity = 0;
    lexer->stack_top = 0;

    if (NT_Lexer_push(lexer, STATE_MARKUP) < 0)
    {
        NT_Lexer_free(lexer);
        lexer = NULL;
        return NULL;
    }

    return lexer;
}

void NT_Lexer_free(NT_Lexer *l)
{
    Py_DECREF(l->str);
    PyMem_Free(l->state);
    l->state = NULL;
    l->stack_capacity = 0;
    l->stack_top = 0;
    l->pos = 0;
    PyMem_Free(l);
}

NT_Token NT_Lexer_next(NT_Lexer *l)
{
    if (l->pos >= l->length)
    {
        return NT_Token_make(l->length, l->length, TOK_EOF);
    }

    LexFn fn = state_table[NT_Lexer_pop(l)];

    if (!fn)
    {
        PyErr_SetString(PyExc_ValueError, "unknown lexer state");
        return NT_Token_make(l->pos, l->pos, TOK_ERROR);
    }

    return fn(l);
}

NT_Token *NT_Lexer_scan(NT_Lexer *l, Py_ssize_t *out_token_count)
{
    // NOLINTNEXTLINE(readability-magic-numbers)
    Py_ssize_t capacity = 128;
    Py_ssize_t token_count = 0;
    NT_Token *tokens = PyMem_Malloc(sizeof(NT_Token) * capacity);

    if (!tokens)
    {
        PyErr_NoMemory();
        return NULL;
    }

    for (;;)
    {
        NT_Token tok = NT_Lexer_next(l);
        if (tok.kind == TOK_ERROR)
        {
            PyMem_Free(tokens);
            return NULL;
        }

        if (token_count >= capacity)
        {
            capacity *= 2;
            NT_Token *tmp = PyMem_Realloc(tokens, sizeof(NT_Token) * capacity);

            if (!tmp)
            {
                PyMem_Free(tokens);
                return NULL;
            }
            tokens = tmp;
        }

        tokens[token_count++] = tok;
        if (tok.kind == TOK_EOF)
        {
            break;
        }
    }

    *out_token_count = token_count;
    return tokens;
}

static NT_Token NT_Lexer_lex_markup(NT_Lexer *l)
{
    Py_ssize_t start = l->pos;

    if (NT_Lexer_accept_str(l, "{{"))
    {
        NT_Lexer_push(l, STATE_EXPR);

        if (is_whitespace_control(NT_Lexer_read_char(l)))
        {
            NT_Lexer_push(l, STATE_WC);
        }

        return NT_Token_make(start, l->pos, TOK_OUT_START);
    }

    if (NT_Lexer_accept_str(l, "{%"))
    {
        NT_Lexer_push(l, STATE_TAG);

        if (is_whitespace_control(NT_Lexer_read_char(l)))
        {
            NT_Lexer_push(l, STATE_WC);
        }

        return NT_Token_make(start, l->pos, TOK_TAG_START);
    }

    return NT_Lexer_lex_other(l);
}

static NT_Token NT_Lexer_lex_tag(NT_Lexer *l)
{
    NT_Lexer_push(l, STATE_EXPR);
    NT_Lexer_accept_while(l, is_space_char);
    Py_ssize_t start = l->pos;

    if (NT_Lexer_accept_keyword(l, "if", 2))
    {
        return NT_Token_make(start, l->pos, TOK_IF_TAG);
    }

    if (NT_Lexer_accept_keyword(l, "elif", 4))
    {
        return NT_Token_make(start, l->pos, TOK_ELIF_TAG);
    }

    if (NT_Lexer_accept_keyword(l, "else", 4))
    {
        return NT_Token_make(start, l->pos, TOK_ELSE_TAG);
    }

    // NOLINTNEXTLINE(readability-magic-numbers)
    if (NT_Lexer_accept_keyword(l, "endif", 5))
    {
        return NT_Token_make(start, l->pos, TOK_ENDIF_TAG);
    }

    if (NT_Lexer_accept_keyword(l, "for", 3))
    {
        return NT_Token_make(start, l->pos, TOK_FOR_TAG);
    }

    // NOLINTNEXTLINE(readability-magic-numbers)
    if (NT_Lexer_accept_keyword(l, "endfor", 6))
    {
        return NT_Token_make(start, l->pos, TOK_ENDFOR_TAG);
    }

    PyErr_SetString(PyExc_RuntimeError, "unknown tag");
    return NT_Token_make(start, start, TOK_ERROR);
}

static NT_Token NT_Lexer_lex_expr(NT_Lexer *l)
{
    NT_Lexer_accept_while(l, is_space_char);
    Py_ssize_t start = l->pos;

    Py_UCS4 ch = NT_Lexer_read_char(l);
    NT_Lexer_push(l, STATE_EXPR);

    switch (ch)
    {
    case '"':
        l->pos++;
        return NT_Lexer_lex_string(l, '"');
    case '\'':
        l->pos++;
        return NT_Lexer_lex_string(l, '\'');
    case '.':
        l->pos++;
        return NT_Token_make(start, l->pos, TOK_DOT);
    case '[':
        l->pos++;
        return NT_Token_make(start, l->pos, TOK_L_BRACKET);
    case ']':
        l->pos++;
        return NT_Token_make(start, l->pos, TOK_R_BRACKET);
    case '(':
        l->pos++;
        return NT_Token_make(start, l->pos, TOK_L_PAREN);
    case ')':
        l->pos++;
        return NT_Token_make(start, l->pos, TOK_R_PAREN);
    case '-':
        l->pos++;
        if (!NT_Lexer_accept_while(l, is_ascii_digit))
        {
            return NT_Token_make(start, l->pos, TOK_WC_HYPHEN);
        }
        // Negative integer
        return NT_Token_make(start, l->pos, TOK_INT);
    case '~':
        l->pos++;
        return NT_Token_make(start, l->pos, TOK_WC_TILDE);
    }

    if (NT_Lexer_accept_while(l, is_ascii_digit))
    {
        return NT_Token_make(start, l->pos, TOK_INT);
    }

    if (NT_Lexer_accept_keyword(l, "and", 3))
    {
        return NT_Token_make(start, l->pos, TOK_AND);
    }

    if (NT_Lexer_accept_keyword(l, "or", 2))
    {
        return NT_Token_make(start, l->pos, TOK_OR);
    }

    if (NT_Lexer_accept_keyword(l, "not", 3))
    {
        return NT_Token_make(start, l->pos, TOK_NOT);
    }

    if (NT_Lexer_accept_keyword(l, "in", 2))
    {
        return NT_Token_make(start, l->pos, TOK_IN);
    }

    if (is_word_char_first(ch))
    {
        l->pos++;
        NT_Lexer_accept_while(l, is_word_char);
        return NT_Token_make(start, l->pos, TOK_WORD);
    }

    return NT_Lexer_lex_end_of_expr(l);
}

static NT_Token NT_Lexer_lex_whitespace_control(NT_Lexer *l)
{
    Py_ssize_t start = l->pos;

    if (NT_Lexer_accept_ch(l, '-'))
    {
        return NT_Token_make(start, l->pos, TOK_WC_HYPHEN);
    }

    if (NT_Lexer_accept_ch(l, '~'))
    {
        return NT_Token_make(start, l->pos, TOK_WC_TILDE);
    }

    PyErr_SetString(PyExc_RuntimeError, "unknown whitespace control");
    return NT_Token_make(l->pos, l->pos, TOK_ERROR); // unreachable
}

static NT_Token NT_Lexer_lex_other(NT_Lexer *l)
{
    Py_ssize_t start = l->pos;

    if (NT_Lexer_accept_until_delim(l))
    {
        return NT_Token_make(start, l->pos, TOK_OTHER);
    }

    // Output extends to the end of the input string.
    l->pos = l->length;
    return NT_Token_make(start, l->pos, TOK_OTHER);
}

static NT_Token NT_Lexer_lex_end_of_expr(NT_Lexer *l)
{
    Py_ssize_t start = l->pos;

    if (NT_Lexer_accept_str(l, "%}"))
    {
        NT_Lexer_pop(l);
        return NT_Token_make(start, l->pos, TOK_TAG_END);
    }

    if (NT_Lexer_accept_str(l, "}}"))
    {
        NT_Lexer_pop(l);
        return NT_Token_make(start, l->pos, TOK_OUT_END);
    }

    l->pos++;
    PyErr_SetString(PyExc_RuntimeError, "unknown token");
    return NT_Token_make(start, l->pos, TOK_ERROR);
}

static NT_Token NT_Lexer_lex_string(NT_Lexer *l, Py_UCS4 quote)
{
    Py_ssize_t start = l->pos;
    Py_UCS4 ch = NT_Lexer_read_char(l);
    NT_TokenKind kind =
        quote == '\'' ? TOK_SINGLE_QUOTE_STRING : TOK_DOUBLE_QUOTE_STRING;

    if (ch == quote)
    {
        // Empty string
        l->pos++;
        return NT_Token_make(start, start, kind);
    }

    for (;;)
    {
        ch = NT_Lexer_read_char(l);

        if (ch == '\\')
        {
            l->pos++;
            kind =
                quote == '\'' ? TOK_SINGLE_ESC_STRING : TOK_DOUBLE_ESC_STRING;
        }
        else if (ch == quote)
        {
            l->pos++;
            return NT_Token_make(start, l->pos - 1, kind);
        }
        else if (ch == (Py_UCS4)-1)
        {
            // end of input
            // unclosed string literal
            PyErr_SetString(PyExc_RuntimeError, "unclosed string literal");
            return NT_Token_make(start, l->pos, TOK_ERROR);
        }

        l->pos++;
    }
}

static inline Py_UCS4 NT_Lexer_read_char(NT_Lexer *l)
{
    // NOTE: PyUnicode_READ_CHAR does give a decent performance boost, but is
    // not part of the stable ABI.
    //
    // Using PyUnicode_AsUCS4Copy and working from the buffer benchmarks about
    // the same as PyUnicode_ReadChar.
    return PyUnicode_ReadChar(l->str, l->pos);
}

static inline Py_UCS4 NT_Lexer_read_char_n(NT_Lexer *l, Py_ssize_t n)
{
    return PyUnicode_ReadChar(l->str, n);
}

static inline bool NT_Lexer_accept_while(NT_Lexer *l, bool (*pred)(Py_UCS4))
{
    Py_ssize_t start = l->pos;
    Py_ssize_t length = l->length;

    while (l->pos < length)
    {
        if (!pred(NT_Lexer_read_char(l)))
        {
            break;
        }
        (l->pos)++;
    }

    return l->pos > start;
}

static inline bool NT_Lexer_accept_ch(NT_Lexer *l, char ch)
{
    Py_ssize_t length = l->length;

    if (l->pos >= length)
    {
        return false;
    }

    if (NT_Lexer_read_char(l) != (unsigned char)ch)
    {
        return false;
    }

    (l->pos)++;
    return true;
}

static inline bool NT_Lexer_accept_str(NT_Lexer *l, const char *sstr)
{
    Py_ssize_t start = l->pos;
    Py_ssize_t length = l->length;
    Py_ssize_t i = 0;

    while (sstr[i] && start + i < length)
    {
        Py_UCS4 ch = NT_Lexer_read_char_n(l, start + i);

        // ASCII-only comparison
        if ((unsigned char)ch != (unsigned char)sstr[i])
        {
            return false;
        }

        i++;
    }

    if (sstr[i] != '\0')
    {
        return false; // string didn't fully match
    }

    l->pos = start + i; // advance the position on success
    return true;
}

static inline bool NT_Lexer_accept_keyword(NT_Lexer *l, const char *word,
                                           Py_ssize_t word_length)
{
    Py_ssize_t start = l->pos;
    Py_ssize_t length = l->length;

    // not enough room for keyword
    if (start + word_length > length)
    {
        return false;
    }

    // check characters one by one
    for (Py_ssize_t i = 0; i < word_length; i++)
    {
        Py_UCS4 ch = NT_Lexer_read_char_n(l, start + i);
        if (ch != (unsigned char)word[i])
        {
            return false;
        }
    }

    // check the boundary after the keyword
    Py_UCS4 next = 0;
    if (start + word_length < length)
    {
        next = NT_Lexer_read_char_n(l, start + word_length);
    }

    if (!is_word_boundary(next))
    {
        return false;
    }

    l->pos += word_length;
    return true;
}

static inline bool NT_Lexer_accept_until_delim(NT_Lexer *l)
{
    Py_ssize_t start = l->pos;
    Py_ssize_t length = l->length;

    while (l->pos < length)
    {
        Py_ssize_t found = PyUnicode_FindChar(l->str, '{', l->pos, length, 1);

        if (found == -1 || found + 1 >= length)
        {
            l->pos = length;
            break;
        }

        Py_UCS4 next = NT_Lexer_read_char_n(l, found + 1);
        if (next == '{' || next == '%')
        {
            l->pos = found;
            break;
        }

        l->pos = found + 1;
    }

    return l->pos > start;
}

// NOLINTBEGIN(readability-magic-numbers)

static inline bool is_ascii_digit(Py_UCS4 ch)
{
    return (ch >= '0' && ch <= '9');
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
            (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
            ch == '_' || ch == '-');
}

// NOLINTEND(readability-magic-numbers)

static int NT_Lexer_push(NT_Lexer *l, NT_State state)
{
    if (l->stack_top >= l->stack_capacity)
    {
        // NOLINTNEXTLINE(readability-magic-numbers)
        Py_ssize_t new_size = l->stack_capacity ? l->stack_capacity * 2 : 8;
        NT_State *new_state = NULL;

        if (!l->state)
        {
            new_state = PyMem_Malloc(sizeof(int) * new_size);
        }
        else
        {
            new_state = PyMem_Realloc(l->state, sizeof(int) * new_size);
        }

        if (!new_state)
        {
            PyErr_NoMemory();
            return -1;
        }

        l->state = new_state;
        l->stack_capacity = new_size;
    }

    l->state[l->stack_top++] = state;
    return 0;
}

static inline NT_State NT_Lexer_pop(NT_Lexer *l)
{
    return l->stack_top ? l->state[--l->stack_top] : STATE_MARKUP;
}
