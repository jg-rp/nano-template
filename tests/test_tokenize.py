from dataclasses import dataclass

from nano_template import _TokenKind as Kind
from nano_template import _TokenView
from nano_template import _tokenize


@dataclass
class _T:
    kind: Kind
    text: str

    def __eq__(self, value: object) -> bool:
        return (
            isinstance(value, _TokenView)
            and value.kind == self.kind
            and value.text == self.text
        )


def test_just_other() -> None:
    text = "hello"
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "hello"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_empty() -> None:
    text = ""
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_just_output() -> None:
    text = "{{ x }}"
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_WORD, "x"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_hello_you() -> None:
    text = "Hello {{ you }}!"
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "Hello "),
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_WORD, "you"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_OTHER, "!"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_if() -> None:
    text = "Hello {% if true %}{{ you }}{% endif %}!"
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "Hello "),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_IF_TAG, "if"),
        _T(Kind.TOK_WORD, "true"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_WORD, "you"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_ENDIF_TAG, "endif"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OTHER, "!"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_if_else() -> None:
    text = "Hello {% if true %}{{ you }}{% else %} guest {% endif %}!"
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "Hello "),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_IF_TAG, "if"),
        _T(Kind.TOK_WORD, "true"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_WORD, "you"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_ELSE_TAG, "else"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OTHER, " guest "),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_ENDIF_TAG, "endif"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OTHER, "!"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_elif() -> None:
    text = "Hello {% if true %}{{ you }}{% elif false %} guest {% endif %}!"
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "Hello "),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_IF_TAG, "if"),
        _T(Kind.TOK_WORD, "true"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_WORD, "you"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_ELIF_TAG, "elif"),
        _T(Kind.TOK_WORD, "false"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OTHER, " guest "),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_ENDIF_TAG, "endif"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OTHER, "!"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_for() -> None:
    text = "Hello {% for x in y %}{{ you }}{% endfor %}!"
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "Hello "),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_FOR_TAG, "for"),
        _T(Kind.TOK_WORD, "x"),
        _T(Kind.TOK_IN, "in"),
        _T(Kind.TOK_WORD, "y"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_WORD, "you"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_ENDFOR_TAG, "endfor"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OTHER, "!"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_whitespace_control() -> None:
    text = "Hello {%- if true ~%}{{~ you -}}{% endif %}!"
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "Hello "),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_WC_HYPHEN, "-"),
        _T(Kind.TOK_IF_TAG, "if"),
        _T(Kind.TOK_WORD, "true"),
        _T(Kind.TOK_WC_TILDE, "~"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_WC_TILDE, "~"),
        _T(Kind.TOK_WORD, "you"),
        _T(Kind.TOK_WC_HYPHEN, "-"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_TAG_START, "{%"),
        _T(Kind.TOK_ENDIF_TAG, "endif"),
        _T(Kind.TOK_TAG_END, "%}"),
        _T(Kind.TOK_OTHER, "!"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_single_quoted_string_literal() -> None:
    text = "Hello {{ 'you' }}!"
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "Hello "),
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_SINGLE_QUOTE_STRING, "you"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_OTHER, "!"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_double_quoted_string_literal() -> None:
    text = 'Hello {{ "you" }}!'
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "Hello "),
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_DOUBLE_QUOTE_STRING, "you"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_OTHER, "!"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_single_quoted_string_literal_with_escapes() -> None:
    text = r"Hello {{ 'yo\\u' }}!"
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "Hello "),
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_SINGLE_ESC_STRING, r"yo\\u"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_OTHER, "!"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got


def test_double_quoted_string_literal_with_escapes() -> None:
    text = r'Hello {{ "yo\\u" }}!'
    tokens = _tokenize(text)

    expect: list[_T] = [
        _T(Kind.TOK_OTHER, "Hello "),
        _T(Kind.TOK_OUT_START, "{{"),
        _T(Kind.TOK_DOUBLE_ESC_STRING, r"yo\\u"),
        _T(Kind.TOK_OUT_END, "}}"),
        _T(Kind.TOK_OTHER, "!"),
        _T(Kind.TOK_EOF, ""),
    ]

    assert len(tokens) == len(expect)
    for want, got in zip(expect, tokens):
        assert want == got
