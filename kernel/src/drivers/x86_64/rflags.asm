global interrupts_enabled
interrupts_enabled:
    pushfq
    pop     rax
    shr     rax, 9
    and     rax, 1
    ret
