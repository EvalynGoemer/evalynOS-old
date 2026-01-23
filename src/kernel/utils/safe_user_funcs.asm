;================================================================================================;
; memcpy                                                                                         ;
;================================================================================================;
global safe_memcpy_asm
global safe_memcpy_asm_start
global safe_memcpy_asm_end
global safe_memcpy_asm_fail
safe_memcpy_asm:
safe_memcpy_asm_start:
    mov rax, rdi
    mov rcx, rdx
    rep movsb

safe_memcpy_asm_end:
    ret

; fault handler jumps here if RIP is between safe_memcpy_asm_start & safe_memcpy_asm_end
safe_memcpy_asm_fail:
    mov rax, -1
    ret

;================================================================================================;
; strlen                                                                                         ;
;================================================================================================;
global safe_strlen_asm
global safe_strlen_asm_start
global safe_strlen_asm_end
global safe_strlen_asm_fail
safe_strlen_asm:
safe_strlen_asm_start:
    xor eax, eax

safe_strlen_loop:
    cmp eax, 4096
    jae safe_strlen_asm_fail

    cmp byte [rdi + rax], 0
    je safe_strlen_asm_end

    inc eax
    jmp safe_strlen_loop

safe_strlen_asm_end:
    ret

; fault handler jumps here if RIP is between safe_strlen_asm_start & safe_strlen_asm_end
safe_strlen_asm_fail:
    mov rax, -1
    ret
