// SPDX-License-Identifier: MIT

#include "nano_template/parser.h"
#include "nano_template/error.h"
#include "nano_template/unescape.h"

/// @brief Operator precedence for our recursive descent parser.
typedef enum
{
    PREC_LOWEST = 1,
    PREC_OR,
    PREC_AND,
    PREC_PRE
} Precedence;

// Bit mask for testing NT_TokenKind membership.
typedef size_t NT_TokenMask;

static const NT_TokenMask END_IF_MASK = ((size_t)1 << TOK_ELSE_TAG) |
                                        ((size_t)1 << TOK_ELIF_TAG) |
                                        ((size_t)1 << TOK_ENDIF_TAG);

static const NT_TokenMask END_FOR_MASK =
    ((size_t)1 << TOK_ELSE_TAG) | ((size_t)1 << TOK_ENDFOR_TAG);

static const NT_TokenMask WHITESPACE_CONTROL_MASK =
    ((size_t)1 << TOK_WC_HYPHEN) | ((size_t)1 << TOK_WC_TILDE);

static const NT_TokenMask BIN_OP_MASK =
    ((size_t)1 << TOK_AND) | ((size_t)1 << TOK_OR);

static const NT_TokenMask TERMINATE_EXPR_MASK =
    ((size_t)1 << TOK_WC_HYPHEN) | ((size_t)1 << TOK_WC_TILDE) |
    ((size_t)1 << TOK_OUT_END) | ((size_t)1 << TOK_TAG_END) |
    ((size_t)1 << TOK_OTHER) | ((size_t)1 << TOK_EOF);

/// @brief Allocate and initialize a new node in parser p's arena.
/// @return A pointer to the new node, or NULL on error.
static NT_Node *NT_Parser_make_node(NT_Parser *p, NT_NodeKind kind);

/// @brief Add node `child` to node `parent`.
/// @return 0 on success, -1 on failure.
static int NT_Parser_add_node(NT_Parser *p, NT_Node *parent, NT_Node *child);

/// @brief Allocate and initialize a new expression in parser p's arena.
/// @return A pointer to the new expression, or NULL on error.
static NT_Expr *NT_Parser_make_expr(NT_Parser *p, NT_ExprKind kind,
                                    NT_Token *token);

/// @brief Add object `obj` to expression `expr`.
/// @return 0 on success, -1 on failure.
static int NT_Parser_add_obj(NT_Parser *p, NT_Expr *expr, PyObject *obj);

/// Return the precedence for the given token kind.
static inline Precedence precedence(NT_TokenKind kind);

/// Advance the parser if the current token is a whitespace control token.
static inline void NT_Parser_skip_wc(NT_Parser *p);

/// Consume and store a whitespace control token for use by the next text
/// block.
static inline void NT_Parser_carry_wc(NT_Parser *p);

/// Return the token at `p->pos` and advance position. Keep returning
/// TOK_EOF if there are no more tokens.
static inline NT_Token *NT_Parser_next(NT_Parser *p);

/// Return the token at `p->pos` without advancing position.
static inline NT_Token *NT_Parser_current(NT_Parser *p);

/// Return the token at `p->pos + 1`.
static inline NT_Token *NT_Parser_peek(NT_Parser *p);

/// Return the token at `p->pos + n`.
static inline NT_Token *NT_Parser_peek_n(NT_Parser *p, Py_ssize_t n);

/// Assert that the token at `p->pos` is of kind `kind` and advance the
/// position.
///
/// Return the token on success. Set a RuntimeError and return NULL on failure.
static inline NT_Token *NT_Parser_eat(NT_Parser *p, NT_TokenKind kind);

/// Consume TOK_TAG_START -> kind -> TOK_TAG_END with optional whitespace
/// control.
///
/// Set and exception and return NULL if we're not at a tag of kind `kind`.
static NT_Token *NT_Parser_eat_empty_tag(NT_Parser *p, NT_TokenKind kind);

/// Assert that we're at a valid expression token.
///
/// Set an exception and return -1 on failure. Return 0 on success.
static inline int NT_Parser_expect_expression(NT_Parser *p);

/// Return true if we're at the start of a tag with kind `kind`.
static inline bool NT_Parser_tag(NT_Parser *p, NT_TokenKind kind);

/// Return true if we're at the start of a tag with kind in `end`.
static inline bool NT_Parser_end_block(NT_Parser *p, NT_TokenMask end);

static int NT_Parser_parse(NT_Parser *p, NT_Node *out_node, NT_TokenMask end);
static NT_Node *NT_Parser_parse_text(NT_Parser *p, NT_Token *token);
static NT_Node *NT_Parser_parse_output(NT_Parser *p);
static NT_Node *NT_Parser_parse_tag(NT_Parser *p);
static NT_Node *NT_Parser_parse_if_tag(NT_Parser *p);
static NT_Node *NT_Parser_parse_elif_tag(NT_Parser *p);
static NT_Node *NT_Parser_parse_else_tag(NT_Parser *p);
static NT_Node *NT_Parser_parse_for_tag(NT_Parser *p);

static NT_Expr *NT_Parser_parse_primary(NT_Parser *p, Precedence prec);
static NT_Expr *NT_Parser_parse_group(NT_Parser *p);
static NT_Expr *NT_Parser_parse_not(NT_Parser *p);
static NT_Expr *NT_Parser_parse_infix(NT_Parser *p, NT_Expr *left);
static NT_Expr *NT_Parser_parse_path(NT_Parser *p);
static PyObject *NT_Parser_parse_identifier(NT_Parser *p);
static PyObject *NT_Parser_parse_bracketed_path_segment(NT_Parser *p);
static PyObject *NT_Parser_parse_shorthand_path_selector(NT_Parser *p);

/// Return a new string. The text in str between token `start` and `end`.
static inline PyObject *NT_Token_text(NT_Token *token, PyObject *str);

/// Return Python string `value` stripped of whitespace according to whitespace
/// control tokens `left` and `right`.
static PyObject *trim(PyObject *value, NT_TokenKind left, NT_TokenKind right);

/// Return true if `kind` is a set in `mask`, false otherwise.
static inline bool NT_Token_member(NT_TokenKind kind, NT_TokenMask mask)
{
    return (mask & ((size_t)1 << kind)) != 0;
}

static const NT_TokenMask PATH_PUNCTUATION_MASK =
    ((size_t)1 << TOK_DOT) | ((size_t)1 << TOK_L_BRACKET);

NT_Parser *NT_Parser_new(NT_Mem *mem, PyObject *str, NT_Token *tokens,
                         Py_ssize_t token_count)
{
    NT_Parser *parser = PyMem_Malloc(sizeof(NT_Parser));
    if (!parser)
    {
        PyErr_NoMemory();
        return NULL;
    }

    Py_INCREF(str);

    parser->mem = mem;
    parser->str = str;
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->pos = 0;
    parser->whitespace_carry = TOK_WC_NONE;
    return parser;
}

void NT_Parser_free(NT_Parser *p)
{
    if (p->tokens)
    {
        PyMem_Free(p->tokens);
        p->tokens = NULL;
    }

    Py_XDECREF(p->str);
    PyMem_Free(p);
}

static NT_Node *NT_Parser_make_node(NT_Parser *p, NT_NodeKind kind)
{
    NT_Node *node = NT_Mem_alloc(p->mem, sizeof(NT_Node));
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

static int NT_Parser_add_node(NT_Parser *p, NT_Node *parent, NT_Node *child)
{
    if (!parent->tail)
    {
        NT_NodePage *page = NT_Mem_alloc(p->mem, sizeof(NT_NodePage));
        if (!page)
        {
            return -1;
        }

        page->next = NULL;
        page->count = 0;
        parent->head = page;
        parent->tail = page;
    }

    if (parent->tail->count == NT_CHILDREN_PER_PAGE)
    {
        NT_NodePage *new_page = NT_Mem_alloc(p->mem, sizeof(NT_NodePage));
        if (!new_page)
        {
            return -1;
        }

        new_page->next = NULL;
        new_page->count = 0;
        parent->tail->next = new_page;
        parent->tail = new_page;
    }

    NT_NodePage *page = parent->tail;
    page->nodes[page->count++] = child;
    return 0;
}

static NT_Expr *NT_Parser_make_expr(NT_Parser *p, NT_ExprKind kind,
                                    NT_Token *token)
{
    NT_Expr *expr = NT_Mem_alloc(p->mem, sizeof(NT_Expr));
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

static int NT_Parser_add_obj(NT_Parser *p, NT_Expr *expr, PyObject *obj)
{
    if (!expr->tail)
    {
        NT_ObjPage *page = NT_Mem_alloc(p->mem, sizeof(NT_ObjPage));
        if (!page)
        {
            return -1;
        }

        page->next = NULL;
        page->count = 0;
        expr->head = page;
        expr->tail = page;
    }

    if (expr->tail->count == NT_OBJ_PRE_PAGE)
    {
        NT_ObjPage *new_page = NT_Mem_alloc(p->mem, sizeof(NT_ObjPage));
        if (!new_page)
        {
            return -1;
        }

        new_page->next = NULL;
        new_page->count = 0;
        expr->tail->next = new_page;
        expr->tail = new_page;
    }

    NT_ObjPage *page = expr->tail;
    page->objs[page->count++] = obj;
    NT_Mem_ref(p->mem, obj);
    return 0;
}

NT_Node *NT_Parser_parse_root(NT_Parser *p)
{
    NT_Node *root = NT_Parser_make_node(p, NODE_ROOT);
    if (!root)
    {
        return NULL;
    }

    if (NT_Parser_parse(p, root, 0) < 0)
    {
        return NULL;
    }

    return root;
}

static int NT_Parser_parse(NT_Parser *p, NT_Node *out_node, NT_TokenMask end)
{
    for (;;)
    {
        // Stop if we're at the end of a block.
        if (NT_Parser_end_block(p, end))
        {
            return 0;
        }

        NT_Token *token = NT_Parser_next(p);
        NT_Node *node = NULL;

        switch (token->kind)
        {
        case TOK_OTHER:
            node = NT_Parser_parse_text(p, token);
            break;

        case TOK_OUT_START:
            node = NT_Parser_parse_output(p);
            break;

        case TOK_TAG_START:
            node = NT_Parser_parse_tag(p);
            break;

        case TOK_EOF:
            return 0;

        default:
            nt_parser_error(token, "unexpected '%s'",
                            NT_TokenKind_str(token->kind));
            return -1;
        }

        if (!node || NT_Parser_add_node(p, out_node, node) < 0)
        {
            return -1;
        }
    }
}

static inline NT_Token *NT_Parser_next(NT_Parser *p)
{
    if (p->pos >= p->token_count)
    {
        // Last token is always EOF
        return &p->tokens[p->token_count - 1];
    }

    return &p->tokens[p->pos++];
}

static inline NT_Token *NT_Parser_current(NT_Parser *p)
{
    if (p->pos >= p->token_count)
    {
        return &p->tokens[p->token_count - 1];
    }

    return &p->tokens[p->pos];
}

static inline NT_Token *NT_Parser_peek(NT_Parser *p)
{
    if (p->pos + 1 >= p->token_count)
    {
        // Last token is always EOF
        return &p->tokens[p->token_count - 1];
    }

    return &p->tokens[p->pos + 1];
}

static inline NT_Token *NT_Parser_peek_n(NT_Parser *p, Py_ssize_t n)
{
    if (p->pos + n >= p->token_count)
    {
        // Last token is always EOF
        return &p->tokens[p->token_count - 1];
    }

    return &p->tokens[p->pos + n];
}

static inline NT_Token *NT_Parser_eat(NT_Parser *p, NT_TokenKind kind)
{
    NT_Token *token = NT_Parser_next(p);
    if (token->kind != kind)
    {
        return nt_parser_error(token, "expected %s, found %s",
                               NT_TokenKind_str(kind),
                               NT_TokenKind_str(token->kind));
    }

    return token;
}

static NT_Token *NT_Parser_eat_empty_tag(NT_Parser *p, NT_TokenKind kind)
{
    if (!NT_Parser_eat(p, TOK_TAG_START))
    {
        return NULL;
    }
    NT_Parser_skip_wc(p);
    NT_Token *token = NT_Parser_eat(p, kind);
    if (!token)
    {
        return NULL;
    }
    NT_Parser_carry_wc(p);
    if (!NT_Parser_eat(p, TOK_TAG_END))
    {
        return NULL;
    }
    return token;
}

static inline int NT_Parser_expect_expression(NT_Parser *p)
{
    NT_Token *token = NT_Parser_current(p);

    if (NT_Token_member(token->kind, TERMINATE_EXPR_MASK))
    {
        nt_parser_error(token, "expected an expression");
        return -1;
    }

    return 0;
}

static inline bool NT_Parser_tag(NT_Parser *p, NT_TokenKind kind)
{
    // Assumes we're at TOK_TAG_START.
    NT_Token *token = NT_Parser_peek(p);

    if (token->kind == kind)
    {
        return true;
    }

    if (NT_Token_member(token->kind, WHITESPACE_CONTROL_MASK))
    {
        return NT_Parser_peek_n(p, 2)->kind == kind;
    }

    return false;
}

static inline bool NT_Parser_end_block(NT_Parser *p, NT_TokenMask end)
{
    // Assumes we're at TOK_TAG_START.
    NT_Token *token = NT_Parser_peek(p);

    if (NT_Token_member(token->kind, WHITESPACE_CONTROL_MASK))
    {
        NT_Token *peeked = NT_Parser_peek_n(p, 2);
        if (NT_Token_member(peeked->kind, end))
        {
            return true;
        }
    }

    if (NT_Token_member(token->kind, end))
    {
        return true;
    }

    return false;
}

static inline void NT_Parser_carry_wc(NT_Parser *p)
{
    NT_Token *token = NT_Parser_current(p);
    if (NT_Token_member(token->kind, WHITESPACE_CONTROL_MASK))
    {
        p->whitespace_carry = token->kind;
        p->pos++;
    }
    else
    {
        p->whitespace_carry = TOK_WC_NONE;
    }
}

static inline void NT_Parser_skip_wc(NT_Parser *p)
{
    NT_Token *token = NT_Parser_current(p);
    if (NT_Token_member(token->kind, WHITESPACE_CONTROL_MASK))
    {
        p->pos++;
    }
}

static inline Precedence precedence(NT_TokenKind kind)
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

static NT_Node *NT_Parser_parse_text(NT_Parser *p, NT_Token *token)
{

    PyObject *str = NT_Token_text(token, p->str);
    if (!str)
    {
        return NULL;
    }

    NT_TokenKind wc_right = TOK_WC_NONE;
    NT_Token *peeked = NT_Parser_peek(p);

    if (NT_Token_member(peeked->kind, WHITESPACE_CONTROL_MASK))
    {
        wc_right = peeked->kind;
    }

    PyObject *trimmed = trim(str, p->whitespace_carry, wc_right);
    Py_DECREF(str);

    if (!trimmed)
    {
        return NULL;
    }

    NT_Node *node = NT_Parser_make_node(p, NODE_TEXT);
    if (!node)
    {
        Py_DECREF(trimmed);
        return NULL;
    }

    node->str = trimmed;
    NT_Mem_steal_ref(p->mem, trimmed);
    return node;
}

static NT_Node *NT_Parser_parse_output(NT_Parser *p)
{
    NT_Parser_skip_wc(p);

    NT_Expr *expr = NT_Parser_parse_primary(p, PREC_LOWEST);
    if (!expr)
    {
        return NULL;
    }

    NT_Parser_carry_wc(p);

    if (!NT_Parser_eat(p, TOK_OUT_END))
    {
        return NULL;
    }

    NT_Node *node = NT_Parser_make_node(p, NODE_OUPUT);
    if (!node)
    {
        return NULL;
    }

    node->expr = expr;
    return node;
}

static NT_Node *NT_Parser_parse_tag(NT_Parser *p)
{
    NT_Parser_skip_wc(p);
    NT_Token *token = NT_Parser_next(p);

    switch (token->kind)
    {
    case TOK_IF_TAG:
        return NT_Parser_parse_if_tag(p);
    case TOK_FOR_TAG:
        return NT_Parser_parse_for_tag(p);
    default:
        return nt_parser_error(token, "unexpected token '%s'",
                               NT_TokenKind_str(token->kind));
    }
}

static NT_Node *NT_Parser_parse_if_tag(NT_Parser *p)
{
    NT_Expr *expr = NULL;
    NT_Node *node = NULL;
    NT_Node *tag = NULL;

    tag = NT_Parser_make_node(p, NODE_IF_TAG);
    if (!tag)
    {
        goto fail;
    }

    node = NT_Parser_make_node(p, NODE_IF_BLOCK);
    if (!node)
    {
        goto fail;
    }

    // Assumes TOK_IF_TAG and WC have already been consumed.
    if (NT_Parser_expect_expression(p) < 0)
    {
        goto fail;
    }

    expr = NT_Parser_parse_primary(p, PREC_LOWEST);
    if (!expr)
    {
        goto fail;
    }

    node->expr = expr;
    expr = NULL;

    NT_Parser_carry_wc(p);
    if (!NT_Parser_eat(p, TOK_TAG_END))
    {
        goto fail;
    }

    if (NT_Parser_parse(p, node, END_IF_MASK) < 0)
    {
        goto fail;
    }

    if (NT_Parser_add_node(p, tag, node) < 0)
    {
        goto fail;
    }
    node = NULL;

    // Zero or more elif blocks.
    while (NT_Parser_tag(p, TOK_ELIF_TAG))
    {
        node = NT_Parser_parse_elif_tag(p);
        if (!node)
        {
            goto fail;
        }

        if (NT_Parser_add_node(p, tag, node) < 0)
        {
            goto fail;
        }

        node = NULL;
    }

    // Optional else block.
    if (NT_Parser_tag(p, TOK_ELSE_TAG))
    {
        node = NT_Parser_parse_else_tag(p);
        if (!node)
        {
            goto fail;
        }

        if (NT_Parser_add_node(p, tag, node) < 0)
        {
            goto fail;
        }
        node = NULL;
    }

    if (!NT_Parser_eat_empty_tag(p, TOK_ENDIF_TAG))
    {
        goto fail;
    }

    return tag;

fail:
    return NULL;
}

static NT_Node *NT_Parser_parse_elif_tag(NT_Parser *p)
{
    NT_Node *node = NULL;
    NT_Expr *expr = NULL;

    node = NT_Parser_make_node(p, NODE_ELIF_BLOCK);
    if (!node)
    {
        goto fail;
    }

    if (!NT_Parser_eat(p, TOK_TAG_START))
    {
        goto fail;
    }

    NT_Parser_skip_wc(p);

    if (!NT_Parser_eat(p, TOK_ELIF_TAG))
    {
        goto fail;
    }

    if (NT_Parser_expect_expression(p) < 0)
    {
        goto fail;
    }

    expr = NT_Parser_parse_primary(p, PREC_LOWEST);
    if (!expr)
    {
        goto fail;
    }

    node->expr = expr;
    expr = NULL;

    NT_Parser_carry_wc(p);

    if (!NT_Parser_eat(p, TOK_TAG_END))
    {
        goto fail;
    }

    if (NT_Parser_parse(p, node, END_IF_MASK) < 0)
    {
        goto fail;
    }

    return node;

fail:
    return NULL;
}

static NT_Node *NT_Parser_parse_else_tag(NT_Parser *p)
{
    NT_Node *node = NT_Parser_make_node(p, NODE_ELSE_BLOCK);

    if (!node)
    {
        goto fail;
    }

    if (!NT_Parser_eat_empty_tag(p, TOK_ELSE_TAG))
    {
        goto fail;
    }

    if (NT_Parser_parse(p, node, END_IF_MASK) < 0)
    {
        goto fail;
    }

    return node;

fail:
    return NULL;
}

static NT_Node *NT_Parser_parse_for_tag(NT_Parser *p)
{
    PyObject *ident = NULL;
    NT_Expr *expr = NULL;
    NT_Node *node = NULL;
    NT_Node *tag = NULL;

    tag = NT_Parser_make_node(p, NODE_FOR_TAG);
    if (!tag)
    {
        return NULL;
    }

    // Assumes TOK_FOR_TAG and WC have already been consumed.
    if (NT_Parser_expect_expression(p) < 0)
    {
        goto fail;
    }

    ident = NT_Parser_parse_identifier(p);
    if (!ident)
    {
        goto fail;
    }

    tag->str = ident;
    NT_Mem_steal_ref(p->mem, ident);
    ident = NULL;

    if (!NT_Parser_eat(p, TOK_IN))
    {
        goto fail;
    }

    if (NT_Parser_expect_expression(p) < 0)
    {
        goto fail;
    }

    expr = NT_Parser_parse_primary(p, PREC_LOWEST);
    if (!expr)
    {
        goto fail;
    }

    tag->expr = expr;
    expr = NULL;

    NT_Parser_carry_wc(p);

    if (!NT_Parser_eat(p, TOK_TAG_END))
    {
        goto fail;
    }

    node = NT_Parser_make_node(p, NODE_FOR_BLOCK);
    if (!node)
    {
        goto fail;
    }

    if (NT_Parser_parse(p, node, END_FOR_MASK) < 0)
    {
        goto fail;
    }

    if (NT_Parser_add_node(p, tag, node) < 0)
    {
        goto fail;
    }

    node = NULL;

    // Optional else block.
    if (NT_Parser_tag(p, TOK_ELSE_TAG))
    {
        node = NT_Parser_make_node(p, NODE_ELSE_BLOCK);
        if (!node)
        {
            goto fail;
        }

        if (!NT_Parser_eat_empty_tag(p, TOK_ELSE_TAG))
        {
            goto fail;
        }

        if (NT_Parser_parse(p, node, END_FOR_MASK) < 0)
        {
            goto fail;
        }

        if (NT_Parser_add_node(p, tag, node) < 0)
        {
            goto fail;
        }
        node = NULL;
    }

    if (!NT_Parser_eat_empty_tag(p, TOK_ENDFOR_TAG))
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

static NT_Expr *NT_Parser_parse_primary(NT_Parser *p, Precedence prec)
{
    NT_Expr *left = NULL;
    NT_Token *token = NT_Parser_current(p);
    NT_TokenKind kind = token->kind;
    PyObject *str = NULL;

    switch (kind)
    {
    case TOK_SINGLE_QUOTE_STRING:
    case TOK_DOUBLE_QUOTE_STRING:
        left = NT_Parser_make_expr(p, EXPR_STR, NULL);
        if (!left)
        {
            goto fail;
        }

        str = NT_Token_text(token, p->str);
        if (!str)
        {
            goto fail;
        }

        if (NT_Parser_add_obj(p, left, str) < 0)
        {
            goto fail;
        }

        Py_DECREF(str);
        str = NULL;
        p->pos++;
        break;
    case TOK_SINGLE_ESC_STRING:
    case TOK_DOUBLE_ESC_STRING:
        left = NT_Parser_make_expr(p, EXPR_STR, NULL);
        if (!left)
        {
            goto fail;
        }

        str = unescape(token, p->str);
        if (!str)
        {
            goto fail;
        }

        if (NT_Parser_add_obj(p, left, str) < 0)
        {
            Py_DECREF(str);
            goto fail;
        }

        Py_DECREF(str);
        str = NULL;
        p->pos++;
        break;
    case TOK_L_PAREN:
        left = NT_Parser_parse_group(p);
        break;
    case TOK_WORD:
    case TOK_L_BRACKET:
        left = NT_Parser_parse_path(p);
        break;
    case TOK_NOT:
        left = NT_Parser_parse_not(p);
        break;
    default:
        nt_parser_error(token, "unexpected %s", NT_TokenKind_str(token->kind));
    }

    if (!left)
    {
        goto fail;
    }

    for (;;)
    {
        token = NT_Parser_current(p);
        kind = token->kind;

        if (kind == TOK_EOF || !NT_Token_member(kind, BIN_OP_MASK) ||
            precedence(kind) < prec)
        {
            break;
        }

        left = NT_Parser_parse_infix(p, left);
        if (!left)
        {
            goto fail;
        }
    }

    return left;

fail:
    Py_XDECREF(str);
    return NULL;
}

static NT_Expr *NT_Parser_parse_group(NT_Parser *p)
{
    if (!NT_Parser_eat(p, TOK_L_PAREN))
    {
        return NULL;
    }

    NT_Expr *expr = NT_Parser_parse_primary(p, PREC_LOWEST);
    if (!expr)
    {
        return NULL;
    }

    if (!NT_Parser_eat(p, TOK_R_PAREN))
    {
        return NULL;
    }

    return expr;
}

static PyObject *NT_Parser_parse_identifier(NT_Parser *p)
{
    NT_Token *token = NT_Parser_eat(p, TOK_WORD);
    if (NT_Token_member(NT_Parser_current(p)->kind, PATH_PUNCTUATION_MASK))
    {
        return nt_parser_error(token, "expected an identifier, found a path");
    }
    return NT_Token_text(token, p->str);
}

static NT_Expr *NT_Parser_parse_not(NT_Parser *p)
{
    if (!NT_Parser_eat(p, TOK_NOT))
    {
        return NULL;
    }

    NT_Expr *not_expr = NT_Parser_make_expr(p, EXPR_NOT, NULL);
    if (!not_expr)
    {
        return NULL;
    }

    NT_Expr *expr = NT_Parser_parse_primary(p, PREC_LOWEST);
    if (!expr)
    {
        return NULL;
    }

    not_expr->right = expr;
    return not_expr;
}

static NT_Expr *NT_Parser_parse_infix(NT_Parser *p, NT_Expr *left)
{
    NT_Token *token = NT_Parser_next(p);
    NT_TokenKind kind = token->kind;
    Precedence prec = precedence(kind);
    NT_Expr *right = NT_Parser_parse_primary(p, prec);
    NT_Expr *infix_expr = NULL;

    if (!right)
    {
        return NULL;
    }

    switch (kind)
    {
    case TOK_AND:
        infix_expr = NT_Parser_make_expr(p, EXPR_AND, NULL);
        break;
    case TOK_OR:
        infix_expr = NT_Parser_make_expr(p, EXPR_OR, NULL);
        break;
    default:
        nt_parser_error(token, "unexpected operator '%s'",
                        NT_TokenKind_str(kind));
    };

    if (!infix_expr)
    {
        return NULL;
    }

    infix_expr->left = left;
    infix_expr->right = right;
    return infix_expr;
}

static NT_Expr *NT_Parser_parse_path(NT_Parser *p)
{
    NT_Token *token = NT_Parser_current(p);
    NT_TokenKind kind = token->kind;
    PyObject *obj = NULL;
    NT_Expr *expr = NULL;
    NT_Expr *result = NULL;

    NT_Token *token_copy = NT_Token_copy(p->mem, token);
    if (!token_copy)
    {
        return NULL;
    }

    expr = NT_Parser_make_expr(p, EXPR_VAR, token_copy);
    if (!expr)
    {
        PyMem_Free(token_copy);
        return NULL;
    }

    if (kind == TOK_WORD)
    {
        p->pos++;
        PyObject *str = NT_Token_text(token, p->str);
        if (!str)
        {
            goto cleanup;
        }

        if (NT_Parser_add_obj(p, expr, str) == -1)
        {
            Py_DECREF(str);
            goto cleanup;
        }
        Py_DECREF(str);
    }

    for (;;)
    {
        kind = NT_Parser_next(p)->kind;
        switch (kind)
        {
        case TOK_L_BRACKET:
            obj = NT_Parser_parse_bracketed_path_segment(p);
            break;
        case TOK_DOT:
            obj = NT_Parser_parse_shorthand_path_selector(p);
            break;
        default:
            p->pos--;
            result = expr;
            goto cleanup;
        }

        if (!obj)
        {
            goto cleanup;
        }

        if (NT_Parser_add_obj(p, expr, obj) == -1)
        {
            goto cleanup;
        }
    }

cleanup:
    Py_XDECREF(obj);
    return result;
}

static PyObject *NT_Parser_parse_bracketed_path_segment(NT_Parser *p)
{
    PyObject *segment = NULL;
    NT_Token *token = NT_Parser_next(p);

    switch (token->kind)
    {
    case TOK_INT:
        segment = PyNumber_Long(NT_Token_text(token, p->str));
        break;
    case TOK_DOUBLE_QUOTE_STRING:
    case TOK_SINGLE_QUOTE_STRING:
        segment = NT_Token_text(token, p->str);
        break;
    case TOK_DOUBLE_ESC_STRING:
    case TOK_SINGLE_ESC_STRING:
        segment = unescape(token, p->str);
        break;
    case TOK_R_BRACKET:
        nt_parser_error(token, "empty bracketed segment");
        break;
    default:
        nt_parser_error(token, "unexpected '%s'",
                        NT_TokenKind_str(token->kind));
        break;
    }

    if (!segment)
    {
        return NULL;
    }

    if (!NT_Parser_eat(p, TOK_R_BRACKET))
    {
        return NULL;
    }

    return segment;
}

static PyObject *NT_Parser_parse_shorthand_path_selector(NT_Parser *p)
{
    PyObject *segment = NULL;
    NT_Token *token = NT_Parser_next(p);

    switch (token->kind)
    {
    case TOK_INT:
        segment = PyNumber_Long(NT_Token_text(token, p->str));
        break;
    case TOK_WORD:
    case TOK_AND:
    case TOK_OR:
    case TOK_NOT:
        segment = NT_Token_text(token, p->str);
        break;
    default:
        nt_parser_error(token, "unexpected '%s'",
                        NT_TokenKind_str(token->kind));
        break;
    }

    return segment;
}

static inline PyObject *NT_Token_text(NT_Token *token, PyObject *str)
{
    return PyUnicode_Substring(str, token->start, token->end);
}

static PyObject *trim(PyObject *value, NT_TokenKind left, NT_TokenKind right)
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