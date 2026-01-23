// SPDX-License-Identifier: MIT

#ifndef NT_VM_H
#define NT_VM_H

#include "nano_template/common.h"
#include "nano_template/compiler.h"
#include "nano_template/string_buffer.h"

#define NT_VM_STACK_SIZE 2048
#define NT_VM_MAX_FRAMES 1024

typedef struct NT_VM
{
    PyObject **constant_pool;
    size_t constant_pool_size;

    uint8_t *instructions;
    size_t instructions_size;

    PyObject *serializer; // Callable[[object], str]
    PyObject *undefined;  // Type[Undefined]

    PyObject **stack;
    size_t sp; // stack pointer

    size_t *frames; // An array of base pointers into the stack
    size_t fp;      // frame pointer

    PyObject *buf; // Output buffer
} NT_VM;

/// @brief Allocate and initialize a new virtual machine.
/// Moves instructions and constants out of `code`.
/// @return The new virtual machine, or NULL on failure with an exception set.
NT_VM *NT_VM_new(NT_Code *code, PyObject *serializer, PyObject *undefined);

void NT_VM_free(NT_VM *vm);

/// @brief Run the virtual machine with user data `data`.
/// Clear the output buffer, reset the stack pointer and frame pointer before
/// running bytecode.
/// @return 0 on success, -1 on failure with an exception set.
int NT_VM_run(NT_VM *vm, PyObject *data);

/// @brief Join buffer items into a single string.
/// @return The joined string, or NULL on failure with an exception set.
PyObject *NT_VM_join(NT_VM *vm);

#endif
