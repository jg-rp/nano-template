import sys
import sysconfig
from pathlib import Path
from setuptools import Extension
from setuptools import setup
from setuptools import find_packages


def collect_sources(base_dir: str = "src") -> list[str]:
    base = Path(base_dir)
    return [str(p) for p in base.rglob("*.c")]


debug_build = "--debug" in sys.argv
if debug_build:
    sys.argv.remove("--debug")

extra_compile_args: list[str] = []
extra_link_args: list[str] = []

if sys.platform == "win32":
    extra_compile_args = ["/O2", "/W3"]
else:
    extra_compile_args = (
        [
            "-std=c99",
            "-O0",
            "-g",
            "-Wall",
            "-Wextra",
        ]
        if debug_build
        else [
            "-std=c99",
            "-O3",
            "-Wall",
            "-Wextra",
        ]
    )

define_macros: list[tuple[str, str | None]] = [
    ("PY_SSIZE_T_CLEAN", None),
]

# See https://docs.python.org/3/howto/free-threading-extensions.html#limited-c-api-and-stable-abi
if not sysconfig.get_config_var("Py_GIL_DISABLED"):
    define_macros.append(("Py_LIMITED_API", "0x03090000"))

ext_modules = [
    Extension(
        "nano_template._nano_template",
        sources=collect_sources(),
        include_dirs=["include"],
        define_macros=define_macros,
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
        py_limited_api=not sysconfig.get_config_var("Py_GIL_DISABLED"),
    )
]

options: dict[str, dict[str, str]] = {}

if not sysconfig.get_config_var("Py_GIL_DISABLED"):
    options["bdist_wheel"] = {"py_limited_api": "cp39"}


setup(
    name="nano_template",
    version="0.1",
    packages=find_packages(where="py"),
    ext_modules=ext_modules,
    package_dir={"": "py"},
    options=options,
)
