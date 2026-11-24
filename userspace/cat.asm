; UNIX Cat (concatenate)
; Program work's only on ring3
; Author: lev445

; (C) MaXOS, by SvinoTeam

%include "syscall.inc"
section .text
global _start


_start:
	; open file (path in rdi)
	lea rdi, [rel filename]
	mov eax, SYS_OPEN
	syscall

	cmp rax, 0
	jl .error_open ; if rax < 0, error
	mov rbx, rax ; save fd to rbx

	; read file to buff
	mov edi, ebx ; fd
	lea rsi, [rel buffer] ; buffer address
	mov edx, 4096
	mov eax, SYS_READ
	syscall

	; check for bytes REALLY read!
	cmp rax, 0
	jle .error_read ; if 0 or error (maybe negative num?)
	mov rcx, rax
	syscall_write buffer, rcx

	; close fd 
	mov edi, ebx
	mov eax, SYS_CLOSE
	syscall

	; successfully :)
	mov eax, SYS_EXIT
	xor edi, edi
	syscall
.error_open:
	syscall_write msg_err_open, msg_err_open_len
	jmp .exit_err
	
.error_read:
	syscall_write msg_err_read, msg_err_read_len
.exit_err:
	mov eax, SYS_EXIT
	mov edi, 1 ; code error: 1
	syscall
	
section .data
filename:         db "test.txt", 0
msg_err_open:     db "cat: cannot open file", 10
msg_err_open_len  equ $ - msg_err_open
msg_err_read:     db "cat: read error or empty file (0 bytes)", 10
msg_err_read_len  equ $ - msg_err_read

section .bss
buffer: resb 4096

