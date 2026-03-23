#pragma once
#include <stdbool.h>

extern char __cpu_local_start;
extern char __cpu_local_end;

// TODO: init AP's GSbase to (alloc_start - __cpu_local_start)
// BSP GSbase=0 works fine to use the section in the binary

#define CPU_LOCAL [[gnu::section(".cpu_local")]]

#define CPU_LOCAL_X86_APPLY_GS(VAR) (*((typeof(VAR) __seg_gs*)(&VAR)))
#define CPU_LOCAL_READ8(VAR)  CPU_LOCAL_X86_APPLY_GS(VAR)
#define CPU_LOCAL_READ16(VAR) CPU_LOCAL_X86_APPLY_GS(VAR)
#define CPU_LOCAL_READ32(VAR) CPU_LOCAL_X86_APPLY_GS(VAR)
#define CPU_LOCAL_READ64(VAR) CPU_LOCAL_X86_APPLY_GS(VAR)
#define CPU_LOCAL_WRITE8(VAR, VAL)  (CPU_LOCAL_X86_APPLY_GS(VAR) = (VAL))
#define CPU_LOCAL_WRITE16(VAR, VAL) (CPU_LOCAL_X86_APPLY_GS(VAR) = (VAL))
#define CPU_LOCAL_WRITE32(VAR, VAL) (CPU_LOCAL_X86_APPLY_GS(VAR) = (VAL))
#define CPU_LOCAL_WRITE64(VAR, VAL) (CPU_LOCAL_X86_APPLY_GS(VAR) = (VAL))

_Static_assert(sizeof(bool) ==  1, "bool must be 1 byte");
#define CPU_LOCAL_READ(VAR)              \
    _Generic((VAR),                      \
        bool:     CPU_LOCAL_READ8(VAR) , \
        uint8_t:  CPU_LOCAL_READ8(VAR) , \
        uint16_t: CPU_LOCAL_READ16(VAR), \
        uint32_t: CPU_LOCAL_READ32(VAR), \
        uint64_t: CPU_LOCAL_READ64(VAR)  \
    )

#define CPU_LOCAL_WRITE(VAR, VAL)              \
    _Generic((VAR),                            \
        bool:     CPU_LOCAL_WRITE8(VAR, VAL) , \
        uint8_t:  CPU_LOCAL_WRITE8(VAR, VAL) , \
        uint16_t: CPU_LOCAL_WRITE16(VAR, VAL), \
        uint32_t: CPU_LOCAL_WRITE32(VAR, VAL), \
        uint64_t: CPU_LOCAL_WRITE64(VAR, VAL)  \
    )
