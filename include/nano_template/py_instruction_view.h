// SPDX-License-Identifier: MIT

#ifndef NTPY_INSTRUCTION_VIEW_H
#define NTPY_INSTRUCTION_VIEW_H

#include "nano_template/common.h"

// TODO:

typedef struct
{
    PyObject_HEAD uint8_t *bytes;
} NTPY_InstructionViewObject;

#endif
