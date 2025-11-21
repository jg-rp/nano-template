#include "micro_liquid/parser.h"
#include "micro_liquid/object_list.h"
#include <stdarg.h>

typedef enum
{
    PREC_LOWEST = 1,
    PREC_OR,
    PREC_AND,
    PREC_PRE
} Precedence;

/// Return the precedence for the given token kind.
static inline Precedence precedence(ML_TokenKind kind);

/// Set a RuntimeError and return NULL.
static void *parser_error(ML_Token *token, const char *fmt, ...);

/// Advance the parser if the current token is a whitespace control token.
static inline void ML_Parser_skip_wc(ML_Parser *self);

/// Consume and store a whitespace control token for use by the next text
/// block.
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

/// Assert that we're at a valid expression token.
///
/// Set an exception and return -1 on failure. Return 0 on success.
static inline Py_ssize_t ML_Parser_expect_expression(ML_Parser *self);

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
static ML_Expr *ML_Parser_parse_primary(ML_Parser *self, Precedence prec);
static ML_Expr *ML_Parser_parse_group(ML_Parser *self);
static ML_Expr *ML_Parser_parse_not(ML_Parser *self);
static ML_Expr *ML_Parser_parse_infix(ML_Parser *self, ML_Expr *left);
static ML_Expr *ML_Parser_parse_path(ML_Parser *self);
static PyObject *ML_Parser_parse_identifier(ML_Parser *self);
static PyObject *ML_Parser_parse_bracketed_path_segment(ML_Parser *self);
static PyObject *ML_Parser_parse_shorthand_path_selector(ML_Parser *self);

static inline PyObject *ML_Token_text(ML_Token *self, PyObject *str);

// Bit masks for testing ML_TokenKind membership.
static const ML_TokenMask END_IF_MASK = ((Py_ssize_t)1 << TOK_ELSE_TAG) |
                                        ((Py_ssize_t)1 << TOK_ELIF_TAG) |
                                        ((Py_ssize_t)1 << TOK_ENDIF_TAG);

static const ML_TokenMask END_FOR_MASK =
    ((Py_ssize_t)1 << TOK_ELSE_TAG) | ((Py_ssize_t)1 << TOK_ENDFOR_TAG);

static const ML_TokenMask WHITESPACE_CONTROL_MASK =
    ((Py_ssize_t)1 << TOK_WC_HYPHEN) | ((Py_ssize_t)1 << TOK_WC_TILDE);

static const ML_TokenMask BIN_OP_MASK =
    ((Py_ssize_t)1 << TOK_AND) | ((Py_ssize_t)1 << TOK_OR);

static const ML_TokenMask TERMINATE_EXPR_MASK =
    ((Py_ssize_t)1 << TOK_WC_HYPHEN) | ((Py_ssize_t)1 << TOK_WC_TILDE) |
    ((Py_ssize_t)1 << TOK_OUT_END) | ((Py_ssize_t)1 << TOK_TAG_END) |
    ((Py_ssize_t)1 << TOK_OTHER) | ((Py_ssize_t)1 << TOK_EOF);

static const ML_TokenMask PATH_PUNCTUATION_MASK =
    ((Py_ssize_t)1 << TOK_DOT) | ((Py_ssize_t)1 << TOK_L_BRACKET);

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

void ML_Parser_dealloc(ML_Parser *self)
{
    ML_Token **tokens = self->tokens;
    Py_ssize_t token_count = self->token_count;

    if (tokens)
    {
        for (Py_ssize_t j = 0; j < token_count; j++)
        {
            if (tokens[j])
            {
                PyMem_Free(tokens[j]);
            }
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
    {
        return NULL;
    }

    for (;;)
    {
        // Stop if we're at the end of a block.
        if (ML_Parser_end_block(self, end))
        {
            return nodes;
        }

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
        {
            goto fail;
        }
    }

fail:
    ML_NodeList_dealloc(nodes);
    return NULL;
}

static inline ML_Token *ML_Parser_next(ML_Parser *self)
{
    if (self->pos >= self->token_count)
    {
        // Last token is always EOF
        return self->tokens[self->token_count - 1];
    }

    return self->tokens[self->pos++];
}

static inline ML_Token *ML_Parser_current(ML_Parser *self)
{
    if (self->pos >= self->token_count)
    {
        return self->tokens[self->token_count - 1];
    }

    return self->tokens[self->pos];
}

static inline ML_Token *ML_Parser_peek(ML_Parser *self)
{
    if (self->pos + 1 >= self->token_count)
    {
        // Last token is always EOF
        return self->tokens[self->token_count - 1];
    }

    return self->tokens[self->pos + 1];
}

static inline ML_Token *ML_Parser_peek_n(ML_Parser *self, Py_ssize_t n)
{
    if (self->pos + n >= self->token_count)
    {
        // Last token is always EOF
        return self->tokens[self->token_count - 1];
    }

    return self->tokens[self->pos + n];
}

static inline ML_Token *ML_Parser_eat(ML_Parser *self, ML_TokenKind kind)
{
    ML_Token *token = ML_Parser_next(self);
    if (token->kind != kind)
    {
        return parser_error(token, "expected %s, found %s",
                            ML_TokenKind_str(kind),
                            ML_TokenKind_str(token->kind));
    }

    return token;
}

static ML_Token *ML_Parser_eat_empty_tag(ML_Parser *self, ML_TokenKind kind)
{
    if (!ML_Parser_eat(self, TOK_TAG_START))
    {
        return NULL;
    }
    ML_Parser_skip_wc(self);
    ML_Token *token = ML_Parser_eat(self, kind);
    if (!token)
    {
        return NULL;
    }
    ML_Parser_carry_wc(self);
    if (!ML_Parser_eat(self, TOK_TAG_END))
    {
        return NULL;
    }
    return token;
}

static inline Py_ssize_t ML_Parser_expect_expression(ML_Parser *self)
{
    ML_Token *token = ML_Parser_current(self);

    if (ML_Token_mask_test(token->kind, TERMINATE_EXPR_MASK))
    {
        parser_error(token, "expected an expression");
        return -1;
    }

    return 0;
}

static inline bool ML_Parser_tag(ML_Parser *self, ML_TokenKind kind)
{
    // Assumes we're at TOK_TAG_START.
    ML_Token *token = ML_Parser_peek(self);

    if (token->kind == kind)
    {
        return true;
    }

    if (ML_Token_mask_test(token->kind, WHITESPACE_CONTROL_MASK))
    {
        return ML_Parser_peek_n(self, 2)->kind == kind;
    }

    return false;
}

static inline bool ML_Parser_end_block(ML_Parser *self, ML_TokenMask end)
{
    // Assumes we're at TOK_TAG_START.
    ML_Token *token = ML_Parser_peek(self);

    if (ML_Token_mask_test(token->kind, WHITESPACE_CONTROL_MASK))
    {
        ML_Token *peeked = ML_Parser_peek_n(self, 2);
        if (ML_Token_mask_test(peeked->kind, end))
        {
            return true;
        }
    }

    if (ML_Token_mask_test(token->kind, end))
    {
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
    {
        self->whitespace_carry = TOK_WC_NONE;
    }
}

static inline void ML_Parser_skip_wc(ML_Parser *self)
{
    ML_Token *token = ML_Parser_current(self);
    if (ML_Token_mask_test(token->kind, WHITESPACE_CONTROL_MASK))
    {
        self->pos++;
    }
}

static inline Precedence precedence(ML_TokenKind kind)
{
    switch (kind)
    {
    case TOK_AND:
        return PREC_AND;
    case TOK_OR:
        return PREC_OR;
    case TOK_NOT:
        return PREC_PRE;
    default:
        return PREC_LOWEST;
    }
}

static ML_Node *ML_Parser_parse_text(ML_Parser *self, ML_Token *token)
{
    // TODO: trim
    PyObject *str = ML_Token_text(token, self->str);
    if (!str)
    {
        return NULL;
    }

    ML_Node *node = ML_Node_new(NODE_TEXT, NULL, 0, NULL, str);
    if (!node)
    {
        return NULL;
    }

    return node;
}

static ML_Node *ML_Parser_parse_output(ML_Parser *self)
{
    ML_Parser_skip_wc(self);

    ML_Expr *expr = ML_Parser_parse_primary(self, PREC_LOWEST);
    if (!expr)
    {
        return NULL;
    }

    ML_Parser_carry_wc(self);

    if (!ML_Parser_eat(self, TOK_OUT_END))
    {
        return NULL;
    }

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
    ML_Expr *expr = NULL;
    ML_NodeList *block = NULL;
    ML_Node *node = NULL;

    ML_NodeList *nodes = ML_NodeList_new();
    if (!nodes)
    {
        return NULL;
    }

    // Assumes TOK_IF_TAG and WC have already been consumed.
    if (ML_Parser_expect_expression(self) < 0)
    {
        goto fail;
    }

    expr = ML_Parser_parse_primary(self, PREC_LOWEST);
    if (!expr)
    {
        goto fail;
    }

    ML_Parser_carry_wc(self);
    if (!ML_Parser_eat(self, TOK_TAG_END))
    {
        goto fail;
    }

    block = ML_Parser_parse(self, END_IF_MASK);
    if (!block)
    {
        goto fail;
    }

    node = ML_Node_new(NODE_IF_BLOCK, block->items, block->size, expr, NULL);
    if (!node)
    {
        goto fail;
    }

    if (ML_NodeList_append(nodes, node) < 0)
    {
        goto fail;
    }

    // Zero or more elif blocks.
    while (ML_Parser_tag(self, TOK_ELIF_TAG))
    {
        if (!ML_Parser_eat(self, TOK_TAG_START))
        {
            goto fail;
        }

        ML_Parser_skip_wc(self);

        if (!ML_Parser_eat(self, TOK_ELIF_TAG))
        {
            goto fail;
        }

        if (ML_Parser_expect_expression(self) < 0)
        {
            goto fail;
        }

        expr = ML_Parser_parse_primary(self, PREC_LOWEST);
        if (!expr)
        {
            goto fail;
        }

        ML_Parser_carry_wc(self);

        if (!ML_Parser_eat(self, TOK_TAG_END))
        {
            goto fail;
        }

        block = ML_Parser_parse(self, END_IF_MASK);
        if (!block)
        {
            goto fail;
        }

        node =
            ML_Node_new(NODE_IF_BLOCK, block->items, block->size, expr, NULL);

        if (!node)
        {
            goto fail;
        }

        if (ML_NodeList_append(nodes, node) < 0)
        {
            goto fail;
        }
    }

    // Optional else block.
    if (ML_Parser_tag(self, TOK_ELSE_TAG))
    {
        if (!ML_Parser_eat_empty_tag(self, TOK_ELSE_TAG))
        {
            goto fail;
        }

        block = ML_Parser_parse(self, END_IF_MASK);
        if (!block)
        {
            goto fail;
        }

        node = ML_Node_new(NODE_ELSE_BLOCK, block->items, block->size, NULL,
                           NULL);

        if (!node)
        {
            goto fail;
        }

        if (ML_NodeList_append(nodes, node) < 0)
        {
            goto fail;
        }
    }

    node = ML_Node_new(NODE_IF_TAG, nodes->items, nodes->size, NULL, NULL);
    if (!node)
    {
        goto fail;
    }

    if (!ML_Parser_eat_empty_tag(self, TOK_ENDIF_TAG))
    {
        goto fail;
    }

    ML_NodeList_disown(block);
    ML_NodeList_disown(nodes);
    return node;

fail:
    if (expr)
    {
        ML_Expression_dealloc(expr);
    }
    if (nodes)
    {
        ML_NodeList_dealloc(nodes);
    }
    if (block)
    {
        ML_NodeList_dealloc(block);
    }
    if (node)
    {
        ML_Node_dealloc(node);
    }
    return NULL;
}

static ML_Node *ML_Parser_parse_for_tag(ML_Parser *self)
{
    PyObject *ident = NULL;
    ML_Expr *expr = NULL;
    ML_NodeList *block = NULL;
    ML_Node *node = NULL;

    ML_NodeList *nodes = ML_NodeList_new();
    if (!nodes)
    {
        return NULL;
    }

    // Assumes TOK_FOR_TAG and WC have already been consumed.
    if (ML_Parser_expect_expression(self) < 0)
    {
        goto fail;
    }

    ident = ML_Parser_parse_identifier(self);
    if (!ident)
    {
        goto fail;
    }

    if (!ML_Parser_eat(self, TOK_IN))
    {
        goto fail;
    }

    if (ML_Parser_expect_expression(self) < 0)
    {
        goto fail;
    }

    expr = ML_Parser_parse_primary(self, PREC_LOWEST);
    if (!expr)
    {
        goto fail;
    }

    ML_Parser_carry_wc(self);

    if (!ML_Parser_eat(self, TOK_TAG_END))
    {
        goto fail;
    }

    block = ML_Parser_parse(self, END_FOR_MASK);
    if (!block)
    {
        goto fail;
    }

    node = ML_Node_new(NODE_FOR_BLOCK, block->items, block->size, NULL, NULL);
    if (!node)
    {
        goto fail;
    }

    if (ML_NodeList_append(nodes, node) < 0)
    {
        goto fail;
    }

    // Optional else block.
    if (ML_Parser_tag(self, TOK_ELSE_TAG))
    {
        if (!ML_Parser_eat_empty_tag(self, TOK_ELSE_TAG))
        {
            goto fail;
        }

        block = ML_Parser_parse(self, END_FOR_MASK);
        if (!block)
        {
            goto fail;
        }

        node = ML_Node_new(NODE_ELSE_BLOCK, block->items, block->size, NULL,
                           NULL);

        if (!node)
        {
            goto fail;
        }

        if (ML_NodeList_append(nodes, node) < 0)
        {
            goto fail;
        }
    }

    if (!ML_Parser_eat_empty_tag(self, TOK_ENDFOR_TAG))
    {
        goto fail;
    }

    node = ML_Node_new(NODE_FOR_TAG, nodes->items, nodes->size, expr, ident);
    if (!node)
    {
        goto fail;
    }

    ML_NodeList_disown(block);
    ML_NodeList_disown(nodes);
    return node;

fail:
    if (ident)
    {
        Py_XDECREF(ident);
    }
    if (expr)
    {
        ML_Expression_dealloc(expr);
    }
    if (block)
    {
        ML_NodeList_dealloc(block);
    }
    if (nodes)
    {
        ML_NodeList_dealloc(nodes);
    }
    if (node)
    {
        ML_Node_dealloc(node);
    }
    return NULL;
}

static ML_Expr *ML_Parser_parse_primary(ML_Parser *self, Precedence prec)
{
    ML_Expr *left = NULL;
    ML_Token *token = ML_Parser_current(self);
    ML_TokenKind kind = token->kind;

    switch (kind)
    {
    case TOK_SINGLE_QUOTE_STRING:
    case TOK_DOUBLE_QUOTE_STRING:
        // XXX: check ML_Token_text return value is not NULL
        left = ML_Expression_new(EXPR_STR, NULL, NULL, 0,
                                 ML_Token_text(token, self->str), NULL, 0);
        self->pos++;
        break;
    case TOK_SINGLE_ESC_STRING:
    case TOK_DOUBLE_ESC_STRING:
        // TODO: unescape
        // XXX: check ML_Token_text return value is not NULL
        left = ML_Expression_new(EXPR_STR, NULL, NULL, 0,
                                 ML_Token_text(token, self->str), NULL, 0);
        self->pos++;
        break;
    case TOK_L_PAREN:
        left = ML_Parser_parse_group(self);
        break;
    case TOK_WORD:
    case TOK_L_BRACKET:
        left = ML_Parser_parse_path(self);
        break;
    case TOK_NOT:
        left = ML_Parser_parse_not(self);
        break;
    default:
        parser_error(token, "unexpected %s", ML_TokenKind_str(token->kind));
    }

    if (!left)
    {
        goto fail;
    }

    for (;;)
    {
        token = ML_Parser_current(self);
        kind = token->kind;

        if (kind == TOK_UNKNOWN) // TODO: TOK_ERR
        {
            parser_error(token, "unknown token");
            goto fail;
        }

        if (kind == TOK_EOF || !ML_Token_mask_test(kind, BIN_OP_MASK) ||
            precedence(kind) < prec)
        {
            break;
        }

        left = ML_Parser_parse_infix(self, left);
        if (!left)
        {
            goto fail;
        }
    }

    return left;

fail:
    if (left)
    {
        ML_Expression_dealloc(left);
    }
    return NULL;
}

static ML_Expr *ML_Parser_parse_group(ML_Parser *self)
{
    if (!ML_Parser_eat(self, TOK_L_PAREN))
    {
        return NULL;
    }

    ML_Expr *expr = ML_Parser_parse_primary(self, PREC_LOWEST);
    if (!expr)
    {
        return NULL;
    }

    if (!ML_Parser_eat(self, TOK_R_PAREN))
    {
        ML_Expression_dealloc(expr);
        return NULL;
    }

    return expr;
}

static PyObject *ML_Parser_parse_identifier(ML_Parser *self)
{
    ML_Token *token = ML_Parser_eat(self, TOK_WORD);
    if (ML_Token_mask_test(ML_Parser_current(self)->kind,
                           PATH_PUNCTUATION_MASK))
    {
        return parser_error(token, "expected an identifier, found a path");
    }
    return ML_Token_text(token, self->str);
}

static ML_Expr *ML_Parser_parse_not(ML_Parser *self)
{
    if (!ML_Parser_eat(self, TOK_NOT))
    {
        return NULL;
    }

    ML_Expr *expr = ML_Parser_parse_primary(self, PREC_LOWEST);
    if (!expr)
    {
        return NULL;
    }

    return ML_Expression_new(EXPR_NOT, NULL, &expr, 1, NULL, NULL, 0);
}

static ML_Expr *ML_Parser_parse_infix(ML_Parser *self, ML_Expr *left)
{
    ML_Token *token = ML_Parser_next(self);
    ML_TokenKind kind = token->kind;
    Precedence prec = precedence(kind);
    ML_Expr *right = ML_Parser_parse_primary(self, prec);
    ML_Expr *expr = NULL;

    if (!right)
    {
        // ML_Expression_dealloc(left);
        return NULL;
    }

    ML_Expr **children = PyMem_Malloc(sizeof(ML_Expr) * 2);
    if (!children)
    {
        PyErr_NoMemory();
        return NULL;
    }

    children[0] = left;
    children[1] = right;

    switch (kind)
    {
    case TOK_AND:
        expr = ML_Expression_new(EXPR_AND, NULL, children, 2, NULL, NULL, 0);
        break;
    case TOK_OR:
        expr = ML_Expression_new(EXPR_OR, NULL, children, 2, NULL, NULL, 0);
        break;
    default:
        parser_error(token, "unexpected operator '%s'",
                     ML_TokenKind_str(kind));
    };

    if (!expr)
    {
        ML_Expression_dealloc(left);
        ML_Expression_dealloc(right);
        return NULL;
    }

    return expr;
}

static ML_Expr *ML_Parser_parse_path(ML_Parser *self)
{
    ML_ObjList *segments = ML_ObjList_new();
    if (!segments)
    {
        return NULL;
    }

    ML_Token *token = ML_Parser_current(self);
    ML_TokenKind kind = token->kind;
    PyObject *obj = NULL;

    if (kind == TOK_WORD)
    {
        self->pos++;
        PyObject *str = ML_Token_text(token, self->str);
        if (!str)
        {
            goto fail;
        }

        if (ML_ObjList_append(segments, str) == -1)
        {
            goto fail;
        }
        Py_DECREF(str);
    }

    for (;;)
    {
        kind = ML_Parser_next(self)->kind;
        switch (kind)
        {
        case TOK_L_BRACKET:
            obj = ML_Parser_parse_bracketed_path_segment(self);
            break;
        case TOK_DOT:
            obj = ML_Parser_parse_shorthand_path_selector(self);
            break;
        default:
            self->pos--;
            ML_Expr *expr = ML_Expression_new(
                EXPR_VAR, ML_Token_new(token->start, token->end, token->kind),
                NULL, 0, NULL, segments->items, segments->size);

            if (!expr)
            {
                goto fail;
            }

            segments->items = NULL;
            segments->size = 0;
            ML_ObjList_disown(segments);
            return expr;
        }

        if (!obj)
        {
            goto fail;
        }

        if (ML_ObjList_append(segments, obj) == -1)
        {
            goto fail;
        }

        Py_DECREF(obj);
    }

fail:
    if (segments)
    {
        ML_ObjList_dealloc(segments);
    }
    Py_XDECREF(obj);
    return NULL;
}

static PyObject *ML_Parser_parse_bracketed_path_segment(ML_Parser *self)
{
    PyObject *segment = NULL;
    ML_Token *token = ML_Parser_next(self);

    switch (token->kind)
    {
    case TOK_INT:
        segment = PyNumber_Long(ML_Token_text(token, self->str));
        break;
    case TOK_DOUBLE_QUOTE_STRING:
    case TOK_SINGLE_QUOTE_STRING:
        segment = ML_Token_text(token, self->str);
        break;
    case TOK_DOUBLE_ESC_STRING:
    case TOK_SINGLE_ESC_STRING:
        // TODO: unescape
        segment = ML_Token_text(token, self->str);
        break;
    case TOK_R_BRACKET:
        parser_error(token, "empty bracketed segment");
        break;
    default:
        parser_error(token, "unexpected '%s'", ML_TokenKind_str(token->kind));
        break;
    }

    if (!segment)
    {
        return NULL;
    }

    if (!ML_Parser_eat(self, TOK_R_BRACKET))
    {
        return NULL;
    }

    return segment;
}

static PyObject *ML_Parser_parse_shorthand_path_selector(ML_Parser *self)
{
    PyObject *segment = NULL;
    ML_Token *token = ML_Parser_next(self);

    switch (token->kind)
    {
    case TOK_INT:
        segment = PyNumber_Long(ML_Token_text(token, self->str));
        break;
    case TOK_WORD:
    case TOK_AND:
    case TOK_OR:
    case TOK_NOT:
        segment = ML_Token_text(token, self->str);
        break;
    default:
        parser_error(token, "unexpected '%s'", ML_TokenKind_str(token->kind));
        break;
    }

    if (!segment)
    {
        return NULL;
    }

    return segment;
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
    {
        goto fail;
    }

    exc_instance =
        PyObject_CallFunctionObjArgs(PyExc_RuntimeError, msg_obj, NULL);
    if (!exc_instance)
    {
        goto fail;
    }

    start_obj = PyLong_FromSsize_t(token->start);
    if (!start_obj)
    {
        goto fail;
    }

    end_obj = PyLong_FromSsize_t(token->end);
    if (!end_obj)
    {
        goto fail;
    }

    if (PyObject_SetAttrString(exc_instance, "start_index", start_obj) < 0 ||
        PyObject_SetAttrString(exc_instance, "stop_index", end_obj) < 0)
    {
        goto fail;
    }

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

ML_NodeList *ML_NodeList_new(void)
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

void ML_NodeList_dealloc(ML_NodeList *self)
{
    for (Py_ssize_t i = 0; i < self->size; i++)
    {
        if (self->items[i])
        {
            ML_Node_dealloc(self->items[i]);
        }
    }
    PyMem_Free(self->items);
    PyMem_Free(self);
}

void ML_NodeList_disown(ML_NodeList *self)
{
    PyMem_Free(self);
}

Py_ssize_t ML_NodeList_grow(ML_NodeList *self)
{
    Py_ssize_t new_cap = (self->capacity == 0) ? 4 : (self->capacity * 2);
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

Py_ssize_t ML_NodeList_append(ML_NodeList *self, ML_Node *node)
{
    if (self->size == self->capacity)
    {
        if (ML_NodeList_grow(self) != 0)
        {
            return -1;
        }
    }

    self->items[self->size++] = node;
    return 0;
}

static inline PyObject *ML_Token_text(ML_Token *self, PyObject *str)
{
    return PyUnicode_Substring(str, self->start, self->end);
}