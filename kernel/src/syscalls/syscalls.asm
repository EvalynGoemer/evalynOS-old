global syscall_handler
extern execute_syscall
extern current_thread
extern syscall_scratch_space

syscall_handler:
    swapgs

    mov [rel gs:syscall_scratch_space], r15
    mov r15, [rel gs:current_thread]
    mov [r15 + 32], rsp
    mov rsp, [r15 + 24]
    mov r15, [rel gs:syscall_scratch_space]

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp

    sti
    call execute_syscall
    cli

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    mov [rel gs:syscall_scratch_space], r15
    mov r15, [rel gs:current_thread]
    mov [r15 + 24], rsp
    mov rsp, [r15 + 32]
    mov r15, [rel gs:syscall_scratch_space]

    swapgs

    o64 sysret
