#include "micro_liquid/parser.h"
#include <stdarg.h>

/// Set a RuntimeError and return NULL.
static void *parser_error(ML_Token *token, const char *fmt, ...);

/// Consume and store a whitespace control token for use by the next text block.
static void ML_Parser_carry_wc(ML_Parser *self);

/// Return the token at `self->pos` and advance the position. Keep returning
/// TOK_EOF if there are no more tokens.
static inline ML_Token *ML_Parser_next(ML_Parser *self);

/// Return the token at `self->pos` without advancing the position.
static inline ML_Token *ML_Parser_current(ML_Parser *self);

/// Return the token at `self->pos + 1`.
static inline ML_Token *ML_Parser_peek(ML_Parser *self);

/// Assert that the token at `self->pos` is of kind `kind` and advance the
/// position.
///
/// Return the token on success. Set a RuntimeError and return NULL on failure.
static inline ML_Token *ML_Parser_eat(ML_Parser *self, ML_TokenKind kind);

/// Node parsing methods.
static ML_Node *ML_Parser_parse_output(ML_Parser *self);
static ML_Node *ML_Parser_parse_if_tag(ML_Parser *self);
static ML_Node *ML_Parser_parse_for_tag(ML_Parser *self);

// Expression parsing methods.
static ML_Expression *ML_Parser_parse_primary(ML_Parser *self);
static ML_Expression *ML_Parser_parse_group(ML_Parser *self);
static ML_Expression *ML_Parser_parse_infix_expression(ML_Parser *self);
static ML_Expression *ML_Parser_parse_path(ML_Parser *self);
static PyObject *ML_Parser_parse_identifier(ML_Parser *self);
static PyObject *ML_Parser_parse_bracketed_path_segment(ML_Parser *self);
static PyObject *ML_Parser_parse_shorthand_path_selector(ML_Parser *self);

// Helpers for building an array of nodes.
static ML_NodeList *ML_NodeList_new(void);
static int ML_NodeList_grow(ML_NodeList *self);
static int ML_NodeList_append(ML_NodeList *self, ML_Node *node);
static int ML_NodeList_extend(ML_NodeList *self, ML_NodeList *nodes);

static const ML_TokenMask END_IF_MASK = ((Py_ssize_t)1 << TOK_ELSE_TAG) |
                                        ((Py_ssize_t)1 << TOK_ELIF_TAG) |
                                        ((Py_ssize_t)1 << TOK_ENDIF_TAG);

static const ML_TokenMask END_FOR_MASK =
    ((Py_ssize_t)1 << TOK_ELSE_TAG) | ((Py_ssize_t)1 << TOK_ENDFOR_TAG);

static const ML_TokenMask WHITESPACE_CONTROL_MASK =
    ((Py_ssize_t)1 << TOK_WC_HYPHEN) | ((Py_ssize_t)1 << TOK_WC_TILDE);

ML_Parser *ML_Parser_new(PyObject *str, ML_Token **tokens,
                         Py_ssize_t token_count)
{
    ML_Parser *parser = PyMem_Malloc(sizeof(ML_Parser));
    if (!parser)
    {
        PyErr_NoMemory();
        return NULL;
    }

    Py_INCREF(str);
    parser->str = str;
    parser->length = PyUnicode_GetLength(str);
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->pos = 0;
    parser->whitespace_carry = TOK_WC_NONE;
    return parser;
}

void ML_Parser_destroy(ML_Parser *self)
{
    // Some expressions steal a token, so we don't want to free those here.
    // Whenever a token is stolen, it must be replaced with NULL in
    // `self->tokens`.

    ML_Token **tokens = self->tokens;
    Py_ssize_t token_count = self->token_count;

    if (tokens)
    {
        for (Py_ssize_t j = 0; j < token_count; j++)
        {
            if (tokens[j])
                PyMem_Free(tokens[j]);
        }
        PyMem_Free(tokens);
    }

    Py_XDECREF(self->str);
    PyMem_Free(self);
}

ML_NodeList *ML_Parser_parse(ML_Parser *self, ML_TokenMask end)
{
    ML_NodeList *nodes = ML_NodeList_new();
    if (!nodes)
        return NULL;

    ML_Token *token;
    ML_TokenKind kind;
    ML_Token *peeked;
    PyObject *str = NULL;
    ML_Node *node = NULL;

    for (;;)
    {
        token = ML_Parser_next(self);
        kind = token->kind;
        peeked = ML_Parser_peek(self);

        switch (kind)
        {
        case TOK_OTHER:
            str = ML_Token_text(token, self->str);
            if (!str)
                return NULL;
            if (ML_NodeList_append(
                    nodes, ML_Node_new(NODE_TEXT, NULL, 0, NULL, str)) < 0)
                return NULL;
            break;
        case TOK_OUT_START:
            if (ML_Token_mask_test(peeked->kind, WHITESPACE_CONTROL_MASK))
                self->pos++;
            node = ML_Parser_parse_output(self);
            if (!node)
                return NULL;
            if (ML_NodeList_append(nodes, node) < 0)
                return NULL;
            break;
        case TOK_TAG_START:
            // TODO: we can safely consume tag name token here
            // Skip whitespace control and update peeked.
            if (ML_Token_mask_test(peeked->kind, WHITESPACE_CONTROL_MASK))
            {
                self->pos++;
                peeked = ML_Parser_peek(self);
            }

            // Stop if at end of block.
            if (ML_Token_mask_test(peeked->kind, end))
            {
                // Backtrack whitespace control.
                if (ML_Token_mask_test(ML_Parser_current(self)->kind,
                                       WHITESPACE_CONTROL_MASK))
                    self->pos--;
                return nodes;
            }

            if (peeked->kind == TOK_IF_TAG)
                node = ML_Parser_parse_if_tag(self);
            else if (peeked->kind == TOK_FOR_TAG)
                node = ML_Parser_parse_for_tag(self);
            else
                return parser_error(peeked, "unexpected token '%s'",
                                    ML_TokenKind_str(peeked->kind));

            if (!node)
                return NULL;

            if (ML_NodeList_append(nodes, node) < 0)
                return NULL;
            break;
        case TOK_EOF:
            return nodes;
        default:
            return parser_error(peeked, "unexpected token '%s'",
                                ML_TokenKind_str(token->kind));
        }
    }
}

static inline ML_Token *ML_Parser_next(ML_Parser *self)
{
    if (self->pos >= self->length)
        // Last token is always EOF
        return self->tokens[self->length - 1];

    return self->tokens[self->pos++];
}

static inline ML_Token *ML_Parser_current(ML_Parser *self)
{
    if (self->pos >= self->length)
        // Last token is always EOF
        return self->tokens[self->length - 1];

    return self->tokens[self->pos];
}

static inline ML_Token *ML_Parser_peek(ML_Parser *self)
{
    if (self->pos + 1 >= self->length)
        // Last token is always EOF
        return self->tokens[self->length - 1];

    return self->tokens[self->pos + 1];
}

static inline ML_Token *ML_Parser_eat(ML_Parser *self, ML_TokenKind kind)
{
    ML_Token *token = ML_Parser_next(self);
    if (token->kind == kind)
        return parser_error(token, "expected %s, found %s",
                            ML_TokenKind_str(kind),
                            ML_TokenKind_str(token->kind));

    return token;
}

static void ML_Parser_carry_wc(ML_Parser *self)
{
    ML_Token *token = ML_Parser_current(self);
    if (ML_Token_mask_test(token->kind, WHITESPACE_CONTROL_MASK))
    {
        self->whitespace_carry = token->kind;
        self->pos++;
    }
    else
        self->whitespace_carry = TOK_WC_NONE;
}

static ML_Node *ML_Parser_parse_output(ML_Parser *self)
{
    ML_Expression *expr = ML_Parser_parse_primary(self);
    if (!expr)
        return NULL;

    ML_Parser_carry_wc(self);

    if (!ML_Parser_eat(self, TOK_OUT_END))
        return NULL;

    return ML_Node_new(NODE_OUPUT, NULL, 0, expr, NULL);
}

static void *parser_error(ML_Token *token, const char *fmt, ...)
{
    PyObject *exc_instance = NULL;
    PyObject *start_obj = NULL;
    PyObject *end_obj = NULL;
    PyObject *msg_obj = NULL;

    va_list vargs;
    va_start(vargs, fmt);
    msg_obj = PyUnicode_FromFormatV(fmt, vargs);
    va_end(vargs);

    if (!msg_obj)
        goto fail;

    exc_instance =
        PyObject_CallFunctionObjArgs(PyExc_RuntimeError, msg_obj, NULL);
    if (!exc_instance)
        goto fail;

    start_obj = PyLong_FromSsize_t(token->start);
    if (!start_obj)
        goto fail;

    end_obj = PyLong_FromSsize_t(token->end);
    if (!end_obj)
        goto fail;

    if (PyObject_SetAttrString(exc_instance, "start_index", start_obj) < 0 ||
        PyObject_SetAttrString(exc_instance, "stop_index", end_obj) < 0)
        goto fail;

    Py_DECREF(start_obj);
    Py_DECREF(end_obj);
    Py_DECREF(msg_obj);

    PyErr_SetObject(PyExc_RuntimeError, exc_instance);
    Py_DECREF(exc_instance);
    return NULL;

fail:
    Py_XDECREF(start_obj);
    Py_XDECREF(end_obj);
    Py_XDECREF(msg_obj);
    Py_XDECREF(exc_instance);
    return NULL;
}

static ML_NodeList *ML_NodeList_new(void)
{
    ML_NodeList *list = PyMem_Malloc(sizeof(ML_NodeList));
    if (!list)
    {
        PyErr_NoMemory();
        return NULL;
    }

    list->size = 0;
    list->capacity = 4;
    list->items = PyMem_Malloc(sizeof(ML_Node *) * list->capacity);
    if (!list->items)
    {
        PyErr_NoMemory();
        PyMem_Free(list);
        return NULL;
    }

    return list;
}

static int ML_NodeList_grow(ML_NodeList *self)
{
    size_t new_cap = (self->capacity == 0) ? 4 : (self->capacity * 2);
    ML_Node **new_items =
        PyMem_Realloc(self->items, sizeof(ML_Node *) * new_cap);
    if (!new_items)
    {
        PyErr_NoMemory();
        return -1;
    }

    self->items = new_items;
    self->capacity = new_cap;
    return 0;
}

static int ML_NodeList_append(ML_NodeList *self, ML_Node *node)
{
    if (self->size == self->capacity)
    {
        if (ML_NodeList_grow(self) != 0)
            return -1;
    }

    self->items[self->size++] = node;
    return 0;
}

static int ML_NodeList_extend(ML_NodeList *self, ML_NodeList *other)
{
    for (size_t i = 0; i < other->size; i++)
    {
        if (ML_NodeList_append(self, other->items[i]) != 0)
            return -1;
    }
    return 0;
}