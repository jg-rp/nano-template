#include "micro_liquid/parser.h"
#include <stdarg.h>

/// @brief Operator precedence for our recursive descent parser.
typedef enum
{
    PREC_LOWEST = 1,
    PREC_OR,
    PREC_AND,
    PREC_PRE
} Precedence;

/// @brief Allocate and initialize a new node in parser p's arena.
/// @return A pointer to the new node, or NULL on error.
static ML_Node *ML_Parser_make_node(ML_Parser *p, ML_NodeKind kind);

/// @brief Add node `child` to node `parent`.
/// @return 0 on success, -1 on failure.
static int ML_Parser_add_node(ML_Parser *p, ML_Node *parent, ML_Node *child);

/// @brief Allocate and initialize a new expression in parser p's arena.
/// @return A pointer to the new expression, or NULL on error.
static ML_Expr *ML_Parser_make_expr(ML_Parser *p, ML_ExprKind kind,
                                    ML_Token *token);

/// @brief Add object `obj` to expression `expr`.
/// @return 0 oon success, -1 on failure.
static int ML_Parser_add_obj(ML_Parser *p, ML_Expr *expr, PyObject *obj);

/// Return the precedence for the given token kind.
static inline Precedence precedence(ML_TokenKind kind);

/// Set a RuntimeError with details from `token` and return NULL.
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
static inline int ML_Parser_expect_expression(ML_Parser *self);

/// Return true if we're at the start of a tag with kind `kind`.
static inline bool ML_Parser_tag(ML_Parser *self, ML_TokenKind kind);

/// Return true if we're at the start of a tag with kind in `end`.
static inline bool ML_Parser_end_block(ML_Parser *self, ML_TokenMask end);

/// Node parsing methods.
static ML_Node *ML_Parser_parse_text(ML_Parser *self, ML_Token *token);
static ML_Node *ML_Parser_parse_output(ML_Parser *self);
static ML_Node *ML_Parser_parse_tag(ML_Parser *self);
static ML_Node *ML_Parser_parse_if_tag(ML_Parser *self);
static ML_Node *ML_Parser_parse_elif_tag(ML_Parser *self);
static ML_Node *ML_Parser_parse_else_tag(ML_Parser *self);
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
static PyObject *trim(PyObject *value, ML_TokenKind left, ML_TokenKind right);

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

ML_Parser *ML_Parser_new(ML_Mem *mem, PyObject *str, ML_Token *tokens,
                         Py_ssize_t token_count)
{
    ML_Parser *parser = PyMem_Malloc(sizeof(ML_Parser));
    if (!parser)
    {
        PyErr_NoMemory();
        return NULL;
    }

    Py_INCREF(str);

    parser->mem = mem;
    parser->str = str;
    parser->length = PyUnicode_GetLength(str);
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->pos = 0;
    parser->whitespace_carry = TOK_WC_NONE;
    return parser;
}

static ML_Node *ML_Parser_make_node(ML_Parser *p, ML_NodeKind kind)
{
    ML_Node *node = ML_Mem_alloc(p->mem, sizeof(ML_Node));
    if (!node)
    {
        return NULL;
    }

    node->kind = kind;
    node->expr = NULL;
    node->head = NULL;
    node->tail = NULL;
    node->str = NULL;
    return node;
}

static int ML_Parser_add_node(ML_Parser *p, ML_Node *parent, ML_Node *child)
{
    if (!parent->tail)
    {
        ML_NodePage *page = ML_Mem_alloc(p->mem, sizeof(ML_NodePage));
        if (!page)
        {
            return -1;
        }

        page->next = NULL;
        page->count = 0;
        parent->head = page;
        parent->tail = page;
    }

    if (parent->tail->count == ML_CHILDREN_PER_PAGE)
    {
        ML_NodePage *new_page = ML_Mem_alloc(p->mem, sizeof(ML_NodePage));
        if (!new_page)
        {
            return -1;
        }

        new_page->next = NULL;
        new_page->count = 0;
        parent->tail->next = new_page;
        parent->tail = new_page;
    }

    ML_NodePage *page = parent->tail;
    page->nodes[page->count++] = child;
    return 0;
}

static ML_Expr *ML_Parser_make_expr(ML_Parser *p, ML_ExprKind kind,
                                    ML_Token *token)
{
    ML_Expr *expr = ML_Mem_alloc(p->mem, sizeof(ML_Expr));
    if (!expr)
    {
        return NULL;
    }

    expr->kind = kind;
    expr->token = token;
    expr->head = NULL;
    expr->tail = NULL;
    expr->left = NULL;
    expr->right = NULL;
    return expr;
}

static int ML_Parser_add_obj(ML_Parser *p, ML_Expr *expr, PyObject *obj)
{
    if (!expr->tail)
    {
        ML_ObjPage *page = ML_Mem_alloc(p->mem, sizeof(ML_ObjPage));
        if (!page)
        {
            return -1;
        }

        page->next = NULL;
        page->count = 0;
        expr->head = page;
        expr->tail = page;
    }

    if (expr->tail->count == ML_OBJ_PRE_PAGE)
    {
        ML_ObjPage *new_page = ML_Mem_alloc(p->mem, sizeof(ML_ObjPage));
        if (!new_page)
        {
            return -1;
        }

        new_page->next = NULL;
        new_page->count = 0;
        expr->tail->next = new_page;
        expr->tail = new_page;
    }

    ML_ObjPage *page = expr->tail;
    page->objs[page->count++] = obj;
    ML_Mem_ref(p->mem, obj);
    return 0;
}

void ML_Parser_dealloc(ML_Parser *p)
{
    if (p->tokens)
    {
        PyMem_Free(p->tokens);
        p->tokens = NULL;
    }

    Py_XDECREF(p->str);
    PyMem_Free(p);
}

ML_Node *ML_Parser_parse_root(ML_Parser *p)
{
    ML_Node *root = ML_Parser_make_node(p, NODE_ROOT);
    if (!root)
    {
        return NULL;
    }

    if (ML_Parser_parse(p, root, 0) < 0)
    {
        return NULL;
    }

    return root;
}

int ML_Parser_parse(ML_Parser *p, ML_Node *out_node, ML_TokenMask end)
{
    for (;;)
    {
        // Stop if we're at the end of a block.
        if (ML_Parser_end_block(p, end))
        {
            return 0;
        }

        ML_Token *token = ML_Parser_next(p);
        ML_Node *node = NULL;

        switch (token->kind)
        {
        case TOK_OTHER:
            node = ML_Parser_parse_text(p, token);
            break;

        case TOK_OUT_START:
            node = ML_Parser_parse_output(p);
            break;

        case TOK_TAG_START:
            node = ML_Parser_parse_tag(p);
            break;

        case TOK_EOF:
            return 0;

        default:
            parser_error(token, "unexpected '%s'",
                         ML_TokenKind_str(token->kind));
            return -1;
        }

        if (!node || ML_Parser_add_node(p, out_node, node) < 0)
        {
            return -1;
        }
    }
}

static inline ML_Token *ML_Parser_next(ML_Parser *self)
{
    if (self->pos >= self->token_count)
    {
        // Last token is always EOF
        return &self->tokens[self->token_count - 1];
    }

    return &self->tokens[self->pos++];
}

static inline ML_Token *ML_Parser_current(ML_Parser *self)
{
    if (self->pos >= self->token_count)
    {
        return &self->tokens[self->token_count - 1];
    }

    return &self->tokens[self->pos];
}

static inline ML_Token *ML_Parser_peek(ML_Parser *self)
{
    if (self->pos + 1 >= self->token_count)
    {
        // Last token is always EOF
        return &self->tokens[self->token_count - 1];
    }

    return &self->tokens[self->pos + 1];
}

static inline ML_Token *ML_Parser_peek_n(ML_Parser *self, Py_ssize_t n)
{
    if (self->pos + n >= self->token_count)
    {
        // Last token is always EOF
        return &self->tokens[self->token_count - 1];
    }

    return &self->tokens[self->pos + n];
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

static inline int ML_Parser_expect_expression(ML_Parser *self)
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

    PyObject *str = ML_Token_text(token, self->str);
    if (!str)
    {
        return NULL;
    }

    ML_TokenKind wc_right = TOK_WC_NONE;
    ML_Token *peeked = ML_Parser_peek(self);

    if (ML_Token_mask_test(peeked->kind, WHITESPACE_CONTROL_MASK))
    {
        wc_right = peeked->kind;
    }

    PyObject *trimmed = trim(str, self->whitespace_carry, wc_right);
    Py_DECREF(str);

    if (!trimmed)
    {
        return NULL;
    }

    ML_Node *node = ML_Parser_make_node(self, NODE_TEXT);
    if (!node)
    {
        Py_DECREF(trimmed);
        return NULL;
    }

    node->str = trimmed;
    ML_Mem_steal_ref(self->mem, trimmed);
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

    ML_Node *node = ML_Parser_make_node(self, NODE_OUPUT);
    if (!node)
    {
        return NULL;
    }

    node->expr = expr;
    return node;
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
    ML_Node *node = NULL;
    ML_Node *tag = NULL;

    tag = ML_Parser_make_node(self, NODE_IF_TAG);
    if (!tag)
    {
        goto fail;
    }

    node = ML_Parser_make_node(self, NODE_IF_BLOCK);
    if (!node)
    {
        goto fail;
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

    node->expr = expr;
    expr = NULL;

    ML_Parser_carry_wc(self);
    if (!ML_Parser_eat(self, TOK_TAG_END))
    {
        goto fail;
    }

    if (ML_Parser_parse(self, node, END_IF_MASK) < 0)
    {
        goto fail;
    }

    if (ML_Parser_add_node(self, tag, node) < 0)
    {
        goto fail;
    }
    node = NULL;

    // Zero or more elif blocks.
    while (ML_Parser_tag(self, TOK_ELIF_TAG))
    {
        node = ML_Parser_parse_elif_tag(self);
        if (!node)
        {
            goto fail;
        }

        if (ML_Parser_add_node(self, tag, node) < 0)
        {
            goto fail;
        }

        node = NULL;
    }

    // Optional else block.
    if (ML_Parser_tag(self, TOK_ELSE_TAG))
    {
        node = ML_Parser_parse_else_tag(self);
        if (!node)
        {
            goto fail;
        }

        if (ML_Parser_add_node(self, tag, node) < 0)
        {
            goto fail;
        }
        node = NULL;
    }

    if (!ML_Parser_eat_empty_tag(self, TOK_ENDIF_TAG))
    {
        goto fail;
    }

    return tag;

fail:
    return NULL;
}

static ML_Node *ML_Parser_parse_elif_tag(ML_Parser *self)
{
    ML_Node *node = NULL;
    ML_Expr *expr = NULL;

    node = ML_Parser_make_node(self, NODE_ELIF_BLOCK);
    if (!node)
    {
        goto fail;
    }

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

    node->expr = expr;
    expr = NULL;

    ML_Parser_carry_wc(self);

    if (!ML_Parser_eat(self, TOK_TAG_END))
    {
        goto fail;
    }

    if (ML_Parser_parse(self, node, END_IF_MASK) < 0)
    {
        goto fail;
    }

    return node;

fail:
    return NULL;
}

static ML_Node *ML_Parser_parse_else_tag(ML_Parser *self)
{
    ML_Node *node = ML_Parser_make_node(self, NODE_ELSE_BLOCK);

    if (!node)
    {
        goto fail;
    }

    if (!ML_Parser_eat_empty_tag(self, TOK_ELSE_TAG))
    {
        goto fail;
    }

    if (ML_Parser_parse(self, node, END_IF_MASK) < 0)
    {
        goto fail;
    }

    return node;

fail:
    return NULL;
}

static ML_Node *ML_Parser_parse_for_tag(ML_Parser *self)
{
    PyObject *ident = NULL;
    ML_Expr *expr = NULL;
    ML_Node *node = NULL;
    ML_Node *tag = NULL;

    tag = ML_Parser_make_node(self, NODE_FOR_TAG);
    if (!tag)
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

    tag->str = ident;
    ML_Mem_steal_ref(self->mem, ident);
    ident = NULL;

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

    tag->expr = expr;
    expr = NULL;

    ML_Parser_carry_wc(self);

    if (!ML_Parser_eat(self, TOK_TAG_END))
    {
        goto fail;
    }

    node = ML_Parser_make_node(self, NODE_FOR_BLOCK);
    if (!node)
    {
        goto fail;
    }

    if (ML_Parser_parse(self, node, END_FOR_MASK) < 0)
    {
        goto fail;
    }

    if (ML_Parser_add_node(self, tag, node) < 0)
    {
        goto fail;
    }

    node = NULL;

    // Optional else block.
    if (ML_Parser_tag(self, TOK_ELSE_TAG))
    {
        node = ML_Parser_make_node(self, NODE_ELSE_BLOCK);
        if (!node)
        {
            goto fail;
        }

        if (!ML_Parser_eat_empty_tag(self, TOK_ELSE_TAG))
        {
            goto fail;
        }

        if (ML_Parser_parse(self, node, END_FOR_MASK) < 0)
        {
            goto fail;
        }

        if (ML_Parser_add_node(self, tag, node) < 0)
        {
            goto fail;
        }
        node = NULL;
    }

    if (!ML_Parser_eat_empty_tag(self, TOK_ENDFOR_TAG))
    {
        goto fail;
    }

    return tag;

fail:
    if (ident)
    {
        Py_XDECREF(ident);
        ident = NULL;
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
    case TOK_SINGLE_ESC_STRING:
    case TOK_DOUBLE_ESC_STRING:
        // TODO: unescape
        left = ML_Parser_make_expr(self, EXPR_STR, NULL);
        if (!left)
        {
            goto fail;
        }

        PyObject *str = ML_Token_text(token, self->str);
        if (!str)
        {
            goto fail;
        }

        if (ML_Parser_add_obj(self, left, str) < 0)
        {
            Py_DECREF(str);
            goto fail;
        }

        Py_DECREF(str);
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

    ML_Expr *not_expr = ML_Parser_make_expr(self, EXPR_NOT, NULL);
    if (!not_expr)
    {
        return NULL;
    }

    ML_Expr *expr = ML_Parser_parse_primary(self, PREC_LOWEST);
    if (!expr)
    {
        return NULL;
    }

    not_expr->right = expr;
    return not_expr;
}

static ML_Expr *ML_Parser_parse_infix(ML_Parser *self, ML_Expr *left)
{
    ML_Token *token = ML_Parser_next(self);
    ML_TokenKind kind = token->kind;
    Precedence prec = precedence(kind);
    ML_Expr *right = ML_Parser_parse_primary(self, prec);
    ML_Expr *infix_expr = NULL;

    if (!right)
    {
        return NULL;
    }

    switch (kind)
    {
    case TOK_AND:
        infix_expr = ML_Parser_make_expr(self, EXPR_AND, NULL);
        break;
    case TOK_OR:
        infix_expr = ML_Parser_make_expr(self, EXPR_OR, NULL);
        break;
    default:
        parser_error(token, "unexpected operator '%s'",
                     ML_TokenKind_str(kind));
    };

    if (!infix_expr)
    {
        return NULL;
    }

    infix_expr->left = left;
    infix_expr->right = right;
    return infix_expr;
}

static ML_Expr *ML_Parser_parse_path(ML_Parser *self)
{
    ML_Token *token = ML_Parser_current(self);
    ML_TokenKind kind = token->kind;
    PyObject *obj = NULL;

    ML_Token *token_ptr = ML_Token_copy(token);
    if (!token_ptr)
    {
        return NULL;
    }

    ML_Expr *expr = ML_Parser_make_expr(self, EXPR_VAR, token_ptr);
    if (!expr)
    {
        PyMem_Free(token_ptr);
        return NULL;
    }

    if (kind == TOK_WORD)
    {
        self->pos++;
        PyObject *str = ML_Token_text(token, self->str);
        if (!str)
        {
            goto fail;
        }

        if (ML_Parser_add_obj(self, expr, str) == -1)
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
            return expr;
        }

        if (!obj)
        {
            goto fail;
        }

        if (ML_Parser_add_obj(self, expr, obj) == -1)
        {
            goto fail;
        }

        Py_DECREF(obj);
    }

fail:
    // TODO:
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

static inline PyObject *ML_Token_text(ML_Token *self, PyObject *str)
{
    return PyUnicode_Substring(str, self->start, self->end);
}

// TODO: refactor
static PyObject *trim(PyObject *value, ML_TokenKind left, ML_TokenKind right)
{
    PyObject *result = NULL;

    if (left == right)
    {
        if (left == TOK_WC_HYPHEN)
        {
            result = PyObject_CallMethod(value, "strip", NULL);
            return result;
        }
        if (left == TOK_WC_TILDE)
        {
            result = PyObject_CallMethod(value, "strip", "s", "\r\n");
            return result;
        }

        Py_INCREF(value);
        return value;
    }

    result = value;
    Py_INCREF(result);

    if (left == TOK_WC_HYPHEN)
    {
        PyObject *tmp = PyObject_CallMethod(result, "lstrip", NULL);
        if (!tmp)
        {
            goto fail;
        }
        Py_DECREF(result);
        result = tmp;
    }
    else if (left == TOK_WC_TILDE)
    {
        PyObject *tmp = PyObject_CallMethod(result, "lstrip", "s", "\r\n");
        if (!tmp)
        {
            goto fail;
        }
        Py_DECREF(result);
        result = tmp;
    }

    if (right == TOK_WC_HYPHEN)
    {
        PyObject *tmp = PyObject_CallMethod(result, "rstrip", NULL);
        if (!tmp)
        {
            goto fail;
        }
        Py_DECREF(result);
        result = tmp;
    }
    else if (right == TOK_WC_TILDE)
    {
        PyObject *tmp = PyObject_CallMethod(result, "rstrip", "s", "\r\n");
        if (!tmp)
        {
            goto fail;
        }
        Py_DECREF(result);
        result = tmp;
    }

    return result;

fail:
    Py_XDECREF(result);
    return NULL;
}