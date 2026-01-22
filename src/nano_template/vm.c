// SPDX-License-Identifier: MIT

#include "nano_template/vm.h"

typedef int (*OpFn)(NT_VM *vm, size_t *ip, PyObject *data);

static int vm_noop(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_text(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_render(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_not(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_constant(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_global(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_selector(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_pop(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_jump(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_jump_if_falsy(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_jump_if_truthy(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_set_local(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_get_local(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_iter_init(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_iter_next(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_enter_frame(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_leave_frame(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_true(NT_VM *vm, size_t *ip, PyObject *data);
static int vm_op_false(NT_VM *vm, size_t *ip, PyObject *data);

static OpFn vm_op_table[] = {
    [NT_OP_NULL] = vm_noop,
    [NT_OP_TEXT] = vm_op_text,
    [NT_OP_RENDER] = vm_op_render,
    [NT_OP_NOT] = vm_op_not,
    [NT_OP_CONSTANT] = vm_op_constant,
    [NT_OP_GLOBAL] = vm_op_global,
    [NT_OP_SELECTOR] = vm_op_selector,
    [NT_OP_POP] = vm_op_pop,
    [NT_OP_JUMP] = vm_op_jump,
    [NT_OP_JUMP_IF_FALSY] = vm_op_jump_if_falsy,
    [NT_OP_JUMP_IF_TRUTHY] = vm_op_jump_if_truthy,
    [NT_OP_SET_LOCAL] = vm_op_set_local,
    [NT_OP_GET_LOCAL] = vm_op_get_local,
    [NT_OP_ITER_INIT] = vm_op_iter_init,
    [NT_OP_ITER_NEXT] = vm_op_iter_next,
    [NT_OP_ENTER_FRAME] = vm_op_enter_frame,
    [NT_OP_LEAVE_FRAME] = vm_op_leave_frame,
    [NT_OP_TRUE] = vm_op_true,
    [NT_OP_FALSE] = vm_op_false,
};

static int vm_push(NT_VM *vm, PyObject *obj);
static PyObject *vm_pop(NT_VM *vm);
static PyObject *vm_peek(NT_VM *vm);
static size_t *vm_current_frame(NT_VM *vm);
static void *vm_push_frame(NT_VM *vm, size_t frame);
static size_t *vm_pop_frame(NT_VM *vm);
