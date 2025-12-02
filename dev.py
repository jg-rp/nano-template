from nano_template import render

source = "{% if a %}a{% else %}b{% else %}c{% endif %}"
data = {"a": False, "b": False}

print(render(None, data))
