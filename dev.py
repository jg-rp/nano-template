import json
import sys
from nano_template import parse

with open("tests/fixtures/001/data.json") as fd:
    data = json.load(fd)

with open("tests/fixtures/001/template.txt") as fd:
    source = fd.read()

# source = """
# START
# {% for x in y %}
#   - {{ x }}
# {% endfor %}
# END
# """

# data = {"y": list(range(10))}

template = parse(source)

before = sys.gettotalrefcount()

for i in range(10000):
    template.render(data)
    if i % 1000 == 0:
        print(f"Iteration {i}, total refcount={sys.gettotalrefcount()}")

after = sys.gettotalrefcount()
print("Refcount delta:", after - before)
