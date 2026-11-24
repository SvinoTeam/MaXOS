BITS 64
default rel

%include "syscall.inc"

section .text
global _start
_start:
    ;mov rax, 1              ; SYS_PUTC
    ;mov rdi, '1 '
    ;syscall
    
    syscall_write msg, msg.len
    syscall_write msg2, msg2.len
    
    syscall_exit 0
.hang:
    jmp .hang

msg:     db "Hey!", 10
.len:    equ $ - msg
msg2:    db "How are you today?",10
.len:    equ $ - msg2

section .note.GNU-stack noalloc noexec nowrite progbits
