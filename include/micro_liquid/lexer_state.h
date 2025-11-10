#ifndef ML_TOKENIZER_STACK_H
#define ML_TOKENIZER_STACK_H

#include "micro_liquid/common.h"

typedef enum
{
    STATE_MARKUP = 1,
    STATE_EXPR,
    STATE_TAG,
    STATE_OUT,
    STATE_OTHER,
    STATE_WC,
} ML_State;

static const int INIT_STACK_SIZE = 16;

typedef struct
{
    ML_State *data;
    Py_ssize_t size; // allocated size
    Py_ssize_t top;  // next free slot
} ML_StateStack;

static inline void ML_StateStack_init(ML_StateStack *stack)
{
    stack->data = NULL;
    stack->size = 0;
    stack->top = 0;
}

static inline void ML_StateStack_clear(ML_StateStack *stack)
{
    PyMem_Free(stack->data);
    stack->data = NULL;
    stack->size = 0;
    stack->top = 0;
}

static inline int ML_StateStack_push(ML_StateStack *stack, ML_State value)
{
    if (stack->top >= stack->size)
    {
        Py_ssize_t new_size = stack->size ? stack->size * 2 : INIT_STACK_SIZE;
        ML_State *new_data =
            (ML_State *)PyMem_Realloc(stack->data, new_size * sizeof(int));
        if (!new_data)
        {
            PyErr_NoMemory();
            return -1;
        }
        stack->data = new_data;
        stack->size = new_size;
    }
    stack->data[stack->top++] = value;
    return 0;
}

static inline ML_State ML_StateStack_pop(ML_StateStack *stack)
{
    return stack->top ? stack->data[--stack->top] : STATE_MARKUP;
}

static inline ML_State ML_StateStack_top(const ML_StateStack *stack)
{
    return stack->top ? stack->data[stack->top - 1] : STATE_MARKUP;
}

static inline bool ML_StateStack_empty(const ML_StateStack *stack)
{
    return stack->top == 0;
}

#endif
