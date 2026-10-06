/* OpenPlay: run main on a stack of our own size. MIT, Copyright (c) 2026 Dalsin Limited. */
#ifndef OP_STACK_H
#define OP_STACK_H
/* Runs body on a new stack of `bytes` when the task's own is smaller. */
int op_main_with_stack(int (*body)(void), unsigned long bytes);
#endif
