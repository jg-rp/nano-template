#include "micro_liquid/parser.h"
#include <stdarg.h>

/// Set a RuntimeError and return NULL.
static void *parser_error(ML_Token *token, const char *fmt, ...);

/// Advance the parser if the current token is a whitespace control token.
static inline void ML_Parser_skip_wc(ML_Parser *self);

/// Consume and store a whitespace control token for use by the next text block.
static inline void ML_Parser_carry_wc(ML_Parser *self);

/// Return the token at `self->pos` and advance position. Keep returning
/// TOK_EOF if there are no more tokens.
static inline ML_Token *ML_Parser_next(ML_Parser *self);

/// Return the token at `self->pos` without advancing position.
static inline ML_Token *ML_Parser_current(ML_Parser *self);

/// Return the token at `self->pos + 1`.
static inline ML_Token *ML_Parser_peek(ML_Parser *self);

/// Return the token at `self->pos + n`.
static inline ML_Token *ML_Parser_peek_n(ML_Parser *self, Py_ssize_t n);

/// Assert that the token at `self->pos` is of kind `kind` and advance the
/// position.
///
/// Return the token on success. Set a RuntimeError and return NULL on failure.
static inline ML_Token *ML_Parser_eat(ML_Parser *self, ML_TokenKind kind);

/// Consume TOK_TAG_START -> kind -> TOK_TAG_END with optional whitespace
/// control.
///
/// Set and exception and return NULL if we're not at a tag of kind `kind`.
static ML_Token *ML_Parser_eat_empty_tag(ML_Parser *self, ML_TokenKind kind);

/// Return true if we're at the start of a tag with kind `kind`.
static inline bool ML_Parser_tag(ML_Parser *self, ML_TokenKind kind);

/// Return true if we're at the start of a tag with kind in `end`.
static inline bool ML_Parser_end_block(ML_Parser *self, ML_TokenMask end);

/// Node parsing methods.
static ML_Node *ML_Parser_parse_text(ML_Parser *self, ML_Token *token);
static ML_Node *ML_Parser_parse_output(ML_Parser *self);
static ML_Node *ML_Parser_parse_tag(ML_Parser *self);
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

// Helpers for building arrays of nodes.
static ML_NodeList *ML_NodeList_new(void);
static void ML_NodeList_destroy(ML_NodeList *self);
static int ML_NodeList_grow(ML_NodeList *self);
static int ML_NodeList_append(ML_NodeList *self, ML_Node *node);

// TODO: Generalized append only list?

// Helpers for building arrays of expressions.
static ML_ExprList *ML_ExprList_new(void);
static void ML_ExprList_destroy(ML_ExprList *self);
static int ML_ExprList_grow(ML_ExprList *self);
static int ML_ExprList_append(ML_ExprList *self, ML_Expression *node);

static const ML_TokenMask END_IF_MASK = ((Py_ssize_t)1 << TOK_ELSE_TAG) |
                                        ((Py_ssize_t)1 << TOK_ELIF_TAG) |
                                        ((Py_ssize_t)1 << TOK_ENDIF_TAG);

static const ML_TokenMask END_FOR_MASK =
    ((Py_ssize_t)1 << TOK_ELSE_TAG) | ((Py_ssize_t)1 << TOK_ENDFOR_TAG);

static const ML_TokenMask WHITESPACE_CONTROL_MASK =
    ((Py_ssize_t)1 << TOK_WC_HYPHEN) | ((Py_ssize_t)1 << TOK_WC_TILDE);

static const ML_TokenMask TERMINATE_EXPR_MASK =
    ((Py_ssize_t)1 << TOK_WC_HYPHEN) | ((Py_ssize_t)1 << TOK_WC_TILDE) |
    ((Py_ssize_t)1 << TOK_OUT_END) | ((Py_ssize_t)1 << TOK_TAG_END) |
    ((Py_ssize_t)1 << TOK_OTHER) | ((Py_ssize_t)1 << TOK_EOF);

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

    for (;;)
    {
        ML_Token *token = ML_Parser_next(self);
        ML_Node *node = NULL;

        switch (token->kind)
        {
        case TOK_OTHER:
            node = ML_Parser_parse_text(self, token);
            break;

        case TOK_OUT_START:
            node = ML_Parser_parse_output(self);
            break;

        case TOK_TAG_START:
            // Stop if we're at the end of a block.
            if (ML_Parser_end_block(self, end))
                return nodes;

            node = ML_Parser_parse_tag(self);
            break;

        case TOK_EOF:
            return nodes;

        default:
            parser_error(token, "unexpected '%s'",
                         ML_TokenKind_str(token->kind));
            goto fail;
        }

        if (!node || ML_NodeList_append(nodes, node) < 0)
            goto fail;
    }

fail:
    ML_NodeList_destroy(nodes);
    return NULL;
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

static inline ML_Token *ML_Parser_peek_n(ML_Parser *self, Py_ssize_t n)
{
    if (self->pos + n >= self->length)
        // Last token is always EOF
        return self->tokens[self->length - 1];

    return self->tokens[self->pos + n];
}

static inline ML_Token *ML_Parser_eat(ML_Parser *self, ML_TokenKind kind)
{
    ML_Token *token = ML_Parser_next(self);
    if (token->kind != kind)
        return parser_error(token, "expected %s, found %s",
                            ML_TokenKind_str(kind),
                            ML_TokenKind_str(token->kind));

    return token;
}

static ML_Token *ML_Parser_eat_empty_tag(ML_Parser *self, ML_TokenKind kind)
{
    if (!ML_Parser_eat(self, TOK_TAG_START))
        return NULL;
    ML_Parser_skip_wc(self);
    ML_Token *token = ML_Parser_eat(self, kind);
    if (!token)
        return NULL;
    ML_Parser_carry_wc(self);
    if (!ML_Parser_eat(self, TOK_TAG_END))
        return NULL;
    return token;
}

static inline bool ML_Parser_tag(ML_Parser *self, ML_TokenKind kind)
{
    // Assumes we're at TOK_TAG_START.
    ML_Token *token = ML_Parser_peek(self);

    if (token->kind == kind)
        return true;

    if (ML_Token_mask_test(token->kind, WHITESPACE_CONTROL_MASK))
        return ML_Parser_peek_n(self, 2)->kind == kind;

    return false;
}

static inline bool ML_Parser_end_block(ML_Parser *self, ML_TokenMask end)
{
    // Assumes TOK_TAG_START has been consumed.
    ML_Token *token = ML_Parser_current(self);

    if (ML_Token_mask_test(token->kind, end))
        return true;

    if (ML_Token_mask_test(token->kind, WHITESPACE_CONTROL_MASK))
    {
        token = ML_Parser_peek(self);
        if (ML_Token_mask_test(token->kind, end))
            return true;
    }

    return false;
}

static inline void ML_Parser_carry_wc(ML_Parser *self)
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

static inline void ML_Parser_skip_wc(ML_Parser *self)
{
    ML_Token *token = ML_Parser_current(self);
    if (ML_Token_mask_test(token->kind, WHITESPACE_CONTROL_MASK))
        self->pos++;
}

static ML_Node *ML_Parser_parse_text(ML_Parser *self, ML_Token *token)
{
    // TODO: trim
    PyObject *str = ML_Token_text(token, self->str);
    if (!str)
        return NULL;

    ML_Node *node = ML_Node_new(NODE_TEXT, NULL, 0, NULL, str);
    if (!node)
        return NULL;

    return node;
}

static ML_Node *ML_Parser_parse_output(ML_Parser *self)
{
    ML_Parser_skip_wc(self);
    ML_Expression *expr = ML_Parser_parse_primary(self);
    if (!expr)
        return NULL;

    ML_Parser_carry_wc(self);

    if (!ML_Parser_eat(self, TOK_OUT_END))
        return NULL;

    return ML_Node_new(NODE_OUPUT, NULL, 0, expr, NULL);
}

static ML_Node *ML_Parser_parse_tag(ML_Parser *self)
{
    ML_Parser_skip_wc(self);
    ML_Token *token = ML_Parser_next(self);

    switch (token->kind)
    {
    case TOK_IF_TAG:
        return ML_Parser_parse_if_tag(self);
    case TOK_FOR_TAG:
        return ML_Parser_parse_for_tag(self);
    default:
        return parser_error(token, "unexpected token '%s'",
                            ML_TokenKind_str(token->kind));
    }
}

static ML_Node *ML_Parser_parse_if_tag(ML_Parser *self)
{
    ML_Expression *expr = NULL;
    ML_NodeList *block = NULL;
    ML_Node *node = NULL;

    ML_NodeList *nodes = ML_NodeList_new();
    if (!nodes)
        return NULL;

    ML_Token *token = ML_Parser_current(self);

    // Assumes TOK_IF_TAG has already been consumed.
    if (ML_Token_mask_test(token->kind, TERMINATE_EXPR_MASK))
        return parser_error(token, "expected an expression");

    expr = ML_Parser_parse_primary(self);
    if (!expr)
        return NULL;

    ML_Parser_carry_wc(self);
    if (!ML_Parser_eat(self, TOK_TAG_END))
        return NULL;

    block = ML_Parser_parse(self, END_IF_MASK);
    if (!block)
        goto fail;

    node = ML_Node_new(NODE_IF_BLOCK, block->items, block->size, expr, NULL);
    if (!node)
        goto fail;

    if (ML_NodeList_append(nodes, node) < 0)
        goto fail;

    while (ML_Parser_tag(self, TOK_ELIF_TAG))
    {
        ML_Parser_eat(self, TOK_TAG_START);
        ML_Parser_skip_wc(self);
        ML_Parser_eat(self, TOK_ELIF_TAG);

        if (ML_Token_mask_test(token->kind, TERMINATE_EXPR_MASK))
            return parser_error(token, "expected an expression");

        expr = ML_Parser_parse_primary(self);
        if (!expr)
            goto fail;

        ML_Parser_carry_wc(self);
        ML_Parser_eat(self, TOK_TAG_END);

        block = ML_Parser_parse(self, END_IF_MASK);
        if (!block)
            goto fail;

        node =
            ML_Node_new(NODE_ELIF_BLOCK, block->items, block->size, expr, NULL);

        if (!node)
            goto fail;

        if (ML_NodeList_append(nodes, node) < 0)
            goto fail;
    }

    if (ML_Parser_tag(self, TOK_ELSE_TAG))
    {
        ML_Parser_eat_empty_tag(self, TOK_ELSE_TAG);
        block = ML_Parser_parse(self, END_IF_MASK);
        if (!block)
            goto fail;

        node =
            ML_Node_new(NODE_ELSE_BLOCK, block->items, block->size, NULL, NULL);

        if (!node)
            goto fail;

        if (ML_NodeList_append(nodes, node) < 0)
            goto fail;
    }

    node = ML_Node_new(NODE_IF_TAG, nodes->items, nodes->size, NULL, NULL);
    if (!node)
        goto fail;

    PyMem_Free(block); // Children take ownership of block items.
    PyMem_Free(nodes); // Node takes ownership of node list items.
    return node;

fail:
    if (expr)
        ML_Expression_destroy(expr);
    if (nodes)
        ML_NodeList_destroy(nodes);
    if (block)
        ML_NodeList_destroy(block);
    if (node)
        ML_Node_destroy(node);
    return NULL;
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

static void ML_NodeList_destroy(ML_NodeList *self)
{
    for (Py_ssize_t i = 0; i < self->size; i++)
    {
        if (self->items[i])
            ML_Node_destroy(self->items[i]);
    }
    PyMem_Free(self->items);
    PyMem_Free(self);
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

static ML_ExprList *ML_ExprList_new(void)
{
    ML_ExprList *list = PyMem_Malloc(sizeof(ML_ExprList));
    if (!list)
    {
        PyErr_NoMemory();
        return NULL;
    }

    list->size = 0;
    list->capacity = 4;
    list->items = PyMem_Malloc(sizeof(ML_Expression *) * list->capacity);
    if (!list->items)
    {
        PyErr_NoMemory();
        PyMem_Free(list);
        return NULL;
    }

    return list;
}

static void ML_ExprList_destroy(ML_ExprList *self)
{
    for (Py_ssize_t i = 0; i < self->size; i++)
    {
        if (self->items[i])
            ML_Expression_destroy(self->items[i]);
    }
    PyMem_Free(self->items);
    PyMem_Free(self);
}

static int ML_ExprList_grow(ML_ExprList *self)
{
    size_t new_cap = (self->capacity == 0) ? 4 : (self->capacity * 2);
    ML_Expression **new_items =
        PyMem_Realloc(self->items, sizeof(ML_Expression *) * new_cap);
    if (!new_items)
    {
        PyErr_NoMemory();
        return -1;
    }

    self->items = new_items;
    self->capacity = new_cap;
    return 0;
}

static int ML_ExprList_append(ML_ExprList *self, ML_Expression *expr)
{
    if (self->size == self->capacity)
    {
        if (ML_ExprList_grow(self) != 0)
            return -1;
    }

    self->items[self->size++] = expr;
    return 0;
}
