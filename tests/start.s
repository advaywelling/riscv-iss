.section .text
.globl _start
_start:
    li sp, 0x80000 
    call main 
    li a7, 93 
    ecall 
    