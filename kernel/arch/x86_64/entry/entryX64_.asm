; entryX64_.asm - MaXOS
; ring0 (kernel) <-> ring3 (user) context switching.
;   1) enter_usermode_asm()       - enter ring 3 (LEGACY)
;   2) syscall_handler            - kernel entry via SYSCALL (MSR IA32_LSTAR)
;   3) syscall_return_to_kernel() - SYS_EXIT: task_exit()
; MSR init lives in ../lib/msr.c, C dispatcher in syscall.c

BITS 64

extern syscall_dispatch
extern kernel_rsp0
extern task_exit                      ; <-- FIX 3

global enter_usermode_asm
global syscall_handler
global syscall_return_to_kernel       ; <-- FIX 1 (было ..._to_handler)
global maxos_user_rsp
global syscall_stack_fault

; GDT (boot/stage2.asm):
; 0x00 null | 0x08 code32 | 0x10 data32 | 0x18 code64 | 0x20 data64
; 0x28 user_data (DPL3) | 0x30 user_code64 (DPL3) | 0x38 TSS (16b)
KERNEL_CS equ 0x18
KERNEL_DS equ 0x20
USER_DS   equ 0x2B   ; 0x28 | RPL 3
USER_CS   equ 0x33   ; 0x30 | RPL 3

section .text

enter_usermode_asm:
	push rbx
	push rbp
	push r12
	push r13
	push r14
	push r15
	lea  rax, [rel .resume]
	mov  [rel kctx_rsp], rsp
	mov  [rel kctx_rip], rax
	cli
	mov  ax, USER_DS
	mov  ds, ax
	mov  es, ax
	mov  fs, ax
	mov  gs, ax
	push qword USER_DS   ; ss
	push rsi             ; rsp
	push qword 0x202     ; rflags: IF=1 + reserved bit1
	push qword USER_CS   ; cs
	push rdi             ; rip
	iretq
.resume:
	pop  r15
	pop  r14
	pop  r13
	pop  r12
	pop  rbp
	pop  rbx
	ret

align 16
syscall_handler:
    mov  [rel maxos_user_rsp], rsp

    mov   rsp, [rel kernel_rsp0]
    test  rsp, rsp
    jnz   .stack_ok
    lea   rsp, [rel emergency_stack_top]
    mov   qword [rel syscall_stack_fault], 1
.stack_ok:
    ; user_rsp put to STACK KERNEL
    push qword [rel maxos_user_rsp]   ; -> syscall_regs_t.user_rsp
    push  r15
    push  r14
    push  r13
    push  r12
    push  r11 ; user RFLAGS
    push  r10
    push  r9
    push  r8
    push  rbp
    push  rcx  ; user RIP
    push  rbx
    push  rdx
    push  rsi
    push  rdi
    push  rax ; rbp here

    mov  rbp, rsp
    and  rsp, -16
    mov  rdi, rbp                     ; syscall_regs_t *r
    sti
    call syscall_dispatch
    cli

    mov  rsp, rbp
    mov  [rsp], rax                   ; result -> saved slot rax
    pop  rax
    pop  rdi
    pop  rsi
    pop  rdx
    pop  rbx
    pop  rcx                          ; user RIP    for SYSRET
    pop  rbp
    pop  r8
    pop  r9
    pop  r10
    pop  r11                          ; user RFLAGS for SYSRET
    pop  r12
    pop  r13
    pop  r14
    pop  r15

    mov  rsp, [rsp]                   ; end slot = user_rsp
    o64 sysret                        ; RIP<-RCX, RFLAGS<-R11, CS/SS<-STAR[63:48]

; void syscall_return_to_kernel(uint64_t exit_code);  noreturn
syscall_return_to_kernel:
    mov  r10, [rel kctx_rsp]
    test r10, r10
    jz   .kill_task

    mov  r11, [rel kctx_rip]
    mov  qword [rel kctx_rsp], 0 
    mov  qword [rel kctx_rip], 0 
    mov  rsp, r10
    mov  rax, rdi
    push rax
    mov  ax, KERNEL_DS
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    pop  rax
    sti
    jmp  r11

.kill_task:
    ; rdi = exit_code -> task_exit(int): ZOMBIE + schedule()
    sub  rsp, 8
    call task_exit
    ud2 ; <- return `ud2` instruction because kernel MUST return RIP 

section .bss
align 16
kctx_rsp:            resq 1
kctx_rip:            resq 1
maxos_user_rsp:      resq 1
syscall_stack_fault: resq 1

align 16
emergency_stack:     resb 4096
emergency_stack_top:

section .note.GNU-stack noalloc noexec nowrite progbits
