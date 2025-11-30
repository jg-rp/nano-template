import sys
from pathlib import Path
from setuptools import Extension
from setuptools import setup
from setuptools import find_packages


def collect_sources(base_dir: str = "src") -> list[str]:
    base = Path(base_dir)
    return [str(p) for p in base.rglob("*.c")]


# Check for debug flag
debug_build = "--debug" in sys.argv
if debug_build:
    sys.argv.remove("--debug")

extra_compile_args = []
extra_link_args = []

if sys.platform == "win32":
    extra_compile_args = ["/O2", "/W3"]
else:
    extra_compile_args = (
        [
            "-O0",
            "-g",
            "-Wall",
            "-Wextra",
            # "-fsanitize=address",
            # "-fno-omit-frame-pointer",
        ]
        if debug_build
        else ["-O3", "-Wall", "-Wextra"]
    )

    # if debug_build:
    #     extra_link_args = [
    #         "-static-libasan",
    #         "-fsanitize=address",
    #     ]


ext_modules = [
    Extension(
        "nano_template._nano_template",
        sources=collect_sources(),
        include_dirs=["include"],
        define_macros=[
            ("PY_SSIZE_T_CLEAN", None),
            ("Py_LIMITED_API", "0x03090000"),  # 3.9
        ],
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
        py_limited_api=True,
    )
]

setup(
    name="nano_template",
    version="0.1",
    packages=find_packages(where="py"),
    ext_modules=ext_modules,
    package_dir={"": "py"},
    options={"bdist_wheel": {"py_limited_api": "cp39"}},
)
