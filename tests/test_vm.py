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
    assert nt.compile_and_render("{{ a and b }}", {"a": False, "b": "B"}) == "False"
