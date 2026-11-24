BITS 64
global switch_context
global task_trampoline
global user_trampoline

; void switch_context(uint64_t *old_rsp_ptr, uint64_t new_rsp);
; Agree of System V AMD64 ABI:
; rdi - first arg, ptr to where save rsp now
; rsi = second arg, new rsp value, what need load

switch_context:
	; first, save callee-saved registers
	push rbp
	push rbx
	push r12
	push r13
	push r14
	push r15
	; save rsp to mem in address from rdi
	mov [rdi], rsp
	; load him
	mov rsp, rsi
	; ....
	pop r15
	pop r14
	pop r13
	pop r12
	pop rbx
	pop rbp
	ret
task_trampoline:
	sti
	sub rsp,8
	call rbx
	add rsp,8
.park:
	hlt
	jmp .park	

; First entry to ring3
; rbx--user RIP (0xA00000 + hdr->entry)
; r12--user RSP (0xE00000)
user_trampoline:
	mov ax, 0x2B ; user_data(0x28) | rpl3
	mov ds, ax
	mov es, ax
	push qword 0x2B ; SS
	push r12 ; user RSP
	push qword 0x202 ; RFLAGS: bit1 reserved=1, bit9 IF=1
	push qword 0x33 ; CS--user_code(0x30) | RPL3 
	push rbx ; RIP
	iretq 
