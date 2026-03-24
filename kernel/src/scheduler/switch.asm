global thread_switch
global thread_init_trampoline
global switch_to_user
extern spinlock_unlock_nil

section .rodata
x87fpu dw 0x0C3F
ssefpu dd 0x1F80

section .text

thread_init_trampoline:
    mov rdi, rbx
    mov rsi, rbp
    mov rdx, r12
    mov rcx, r13
    mov r8,  r14
    mov r9,  r15
    ret

thread_switch:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov [rdx], rsp
    mov rsp, rcx
    call spinlock_unlock_nil

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

switch_to_user:
    cli
    swapgs
    fninit
    fldcw [x87fpu]
    ldmxcsr [ssefpu]

    mov r11, 0x202
    mov rcx, rdi
    mov rsp, rsi

    xor eax, eax
    xor ebx, ebx
    xor edx, edx
    xor esi, esi
    xor edi, edi
    xor ebp, ebp
    xor r8d,  r8d
    xor r9d,  r9d
    xor r10d, r10d
    xor r12d, r12d
    xor r13d, r13d
    xor r14d, r14d
    xor r15d, r15d
    o64 sysret
