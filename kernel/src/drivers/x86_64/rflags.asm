global interrupts_enabled
interrupts_enabled:
    pushfq
    pop     rax
    shr     eax, 9
    and     eax, 1
    ret
