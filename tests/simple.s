.section .text
.globl _start
_start:
    addi a0, x0, 42     # exit code 42
    addi a7, x0, 93     # exit syscall
    ecall