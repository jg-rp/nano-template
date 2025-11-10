import sys
from pathlib import Path
from setuptools import Extension
from setuptools import setup
from setuptools import find_packages


def collect_sources(base_dir: str = "src") -> list[str]:
    base = Path(base_dir)
    # Recursively find all .c files
    return [str(p) for p in base.rglob("*.c")]


# Check for debug flag
debug_build = "--debug" in sys.argv
if debug_build:
    sys.argv.remove("--debug")

extra_compile_args = []
if sys.platform == "win32":
    extra_compile_args = ["/O2", "/W3"]
else:
    extra_compile_args = (
        ["-O0", "-g", "-Wall", "-Wextra"]
        if debug_build
        else ["-O3", "-Wall", "-Wextra"]
    )


ext_modules = [
    Extension(
        "micro_liquid._micro_liquid",
        sources=collect_sources(),
        include_dirs=["include"],
        define_macros=[("PY_SSIZE_T_CLEAN", None), ("Py_LIMITED_API", "0x03060000")],
        extra_compile_args=extra_compile_args,
        py_limited_api=True,
    )
]

setup(
    name="micro_liquid",
    version="0.1",
    packages=find_packages(where="py"),
    ext_modules=ext_modules,
    package_dir={"": "py"},
)
