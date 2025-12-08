import sys
from nano_template import render

before = sys.gettotalrefcount()

for i in range(10000):
    render("{% if a %}a{% else %}c{% endif %}", {"a": False, "b": False})
    if i % 1000 == 0:
        print(f"Iteration {i}, total refcount={sys.gettotalrefcount()}")

after = sys.gettotalrefcount()
print("Refcount delta:", after - before)
