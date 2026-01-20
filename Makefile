BUILD_DIR := build
DIST_DIR := dist

PYTHON := uv run python
CLANG_TIDY := clang-tidy
BEAR := bear

SRC_DIR := src
C_SOURCES := $(shell find $(SRC_DIR) -name '*.c')
COMPILE_DB := $(BUILD_DIR)/compile_commands.json

# For use with lldb
VENV_PY := ./.venv/bin/python
# Default test script
TEST ?= dev.py

# Default target: build the C extension in place (optimized)
all: build

# Build optimized version
build:
	$(PYTHON) setup.py build_ext --inplace --force

# Build editable package
develop:
	uv pip install -e .

# Build debug version
build_debug:
	$(PYTHON) setup.py build_ext --inplace --force --debug

# Run Valgrind on the debug build
valgrind: build_debug
	@echo "==> Running Valgrind on _nano_template extension..."
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes \
		$(PYTHON) dev.py

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) py/*.egg-info py/nano_template/*.so ${DIST_DIR}/*

# Full rebuild
rebuild: clean build

# Full rebuild debug
rebuild_debug: clean build_debug

# Format C code
format:
	clang-format -i src/**/*.c include/**/*.h

# Run clang-tidy using compile_commands.json
tidy: $(COMPILE_DB)
	@echo "==> Running clang-tidy..."
	$(CLANG_TIDY) -p $(BUILD_DIR) $(C_SOURCES)

# Generate compile_commands.json using Bear (only if missing or outdated)
$(COMPILE_DB): setup.py $(C_SOURCES)
	@echo "==> Generating compile_commands.json with Bear..."
	mkdir -p $(BUILD_DIR)
	$(BEAR) --output $(COMPILE_DB) -- $(PYTHON) setup.py build_ext --inplace --force

# Run tests
test:
	$(PYTHON) -m pytest -v

lldb: rebuild_debug
	lldb-16 -- $(VENV_PY) $(TEST)

.PHONY: all build build_debug develop clean rebuild rebuild_debug format tidy valgrind test
