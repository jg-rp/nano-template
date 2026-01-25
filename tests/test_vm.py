import nano_template as nt


def test_just_text() -> None:
    assert nt.compile_and_render("Hello, World!", {}) == "Hello, World!"


def test_empty_template() -> None:
    assert nt.compile_and_render("", {}) == ""


def test_just_output() -> None:
    assert nt.compile_and_render("{{ a }}", {"a": "A"}) == "A"


def test_just_output_dotted_path() -> None:
    assert nt.compile_and_render("{{ a.b }}", {"a": {"b": "B"}}) == "B"


def test_logical_or_truthy_left() -> None:
    assert nt.compile_and_render("{{ a or b }}", {"a": "A", "b": "B"}) == "A"


def test_logical_or_falsy_left() -> None:
    assert nt.compile_and_render("{{ a or b }}", {"a": False, "b": "B"}) == "B"


def test_logical_and_truthy_left() -> None:
    assert nt.compile_and_render("{{ a and b }}", {"a": "A", "b": "B"}) == "B"


def test_logical_and_falsy_left() -> None:
    assert (
        nt.compile_and_render(
            "{{ a and b }}",
            {"a": False, "b": "B"},
        )
        == "False"
    )


def test_logical_not_false() -> None:
    assert (
        nt.compile_and_render(
            "{{ not a }}",
            {"a": False},
        )
        == "True"
    )


def test_logical_not_true() -> None:
    assert (
        nt.compile_and_render(
            "{{ not a }}",
            {"a": True},
        )
        == "False"
    )


def test_condition_truthy() -> None:
    assert (
        nt.compile_and_render(
            "{% if a %}b{% endif %}",
            {"a": True},
        )
        == "b"
    )


def test_condition_falsy() -> None:
    assert (
        nt.compile_and_render(
            "{% if a %}b{% endif %}",
            {"a": False},
        )
        == ""
    )


def test_condition_truthy_alternative() -> None:
    assert (
        nt.compile_and_render(
            "{% if a %}b{% else %}c{% endif %}",
            {"a": True},
        )
        == "b"
    )


def test_condition_falsy_alternative() -> None:
    assert (
        nt.compile_and_render(
            "{% if a %}b{% else %}c{% endif %}",
            {"a": False},
        )
        == "c"
    )


def test_conditions_false_true() -> None:
    assert (
        nt.compile_and_render(
            "{% if a %}b{% elif c %}d{% endif %}",
            {"a": False, "c": True},
        )
        == "d"
    )


def test_conditions_true_true() -> None:
    assert (
        nt.compile_and_render(
            "{% if a %}b{% elif c %}d{% endif %}",
            {"a": True, "c": True},
        )
        == "b"
    )


def test_conditions_with_alternative_true() -> None:
    assert (
        nt.compile_and_render(
            "{% if a %}b{% elif c %}d{% else %}e{% endif %}!",
            {"a": True, "c": True},
        )
        == "b!"
    )


def test_conditions_with_alternative_false() -> None:
    assert (
        nt.compile_and_render(
            "{% if a %}b{% elif c %}d{% else %}e{% endif %}!",
            {"a": False, "c": False},
        )
        == "e!"
    )


def test_loop() -> None:
    assert (
        nt.compile_and_render(
            "{% for x in y %}{{ x }} {% endfor %}",
            {"y": ["a", "b", "c"]},
        )
        == "a b c "
    )


def test_loop_nested() -> None:
    assert (
        nt.compile_and_render(
            "{% for x in y %}{% for a in x %}{{ a }}{% endfor %}{% endfor %}",
            {"y": [["a", "b"], [2, 3, 4, 5], [3]]},
        )
        == "ab23453"
    )


def test_loop_else_empty() -> None:
    assert (
        nt.compile_and_render(
            "{% for x in y %}{{ x }}{% else %}foo{% endfor %}",
            {"y": []},
        )
        == "foo"
    )


def test_loop_else_not_empty() -> None:
    assert (
        nt.compile_and_render(
            "{% for x in y %}{{ x }}{% else %}foo{% endfor %}",
            {"y": ["a", "b"]},
        )
        == "ab"
    )


def test_loop_else_not_iterable() -> None:
    assert (
        nt.compile_and_render(
            "{% for x in y %}{{ x }}{% else %}foo{% endfor %}",
            {"y": 42},
        )
        == "foo"
    )
