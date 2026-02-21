global thread_switch
global thread_switch_user
global switch_to_user
extern spinlock_unlock_nil

section .rodata
x87fpu dw 0x0C3F
ssefpu dd 0x1F80

section .text
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

USER_STACK_TOP equ 0x0000000080000000

switch_to_user:
    cli
    swapgs
    fninit
    fldcw [x87fpu]
    ldmxcsr [ssefpu]

    mov r11, 0x202
    mov rcx, rdi
    mov rsp, rsi
    o64 sysret
