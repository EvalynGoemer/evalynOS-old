# This file was taken and modified from https://codeberg.org/Limine/limine-c-template/raw/commit/c8bc5a2b93397a19272a19a6004b0eeb1e90d982/kernel/GNUmakefile

ARCH := x86_64
CFLAGS := -O2 -g -fno-omit-frame-pointer -DMUTE_KERNEL_PANIC
CPPFLAGS :=

.SUFFIXES:

override OUTPUT := kernel.elf

ifeq ($(filter $(ARCH), x86_64),)
    $(error Architecture $(ARCH) not supported)
endif

TOOLCHAIN :=llvm
TOOLCHAIN_PREFIX :=
ifneq ($(TOOLCHAIN),)
    ifeq ($(TOOLCHAIN_PREFIX),)
        TOOLCHAIN_PREFIX := $(TOOLCHAIN)-
        TOOLCHAIN_PREFIX := $(TOOLCHAIN)
    endif
endif

ifneq ($(TOOLCHAIN_PREFIX),)
    CC := $(TOOLCHAIN_PREFIX)clang
else
    CC := cc
endif

LD := $(TOOLCHAIN_PREFIX)ld

ifeq ($(TOOLCHAIN),llvm)
    CC := clang
    LD := ld.lld
endif

ifeq ($(ARCH),x86_64)
    NASMFLAGS := -g
endif

LDFLAGS :=

override CC_IS_CLANG := $(shell ! $(CC) --version 2>/dev/null | grep -q '^Target: '; echo $$?)

override CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu23 \
    -ffreestanding \
    -flto \
    -fstack-protector-all \
    -fsanitize=undefined \
    -fno-PIC \
    -fno-pie \
    -ffunction-sections \
    -fdata-sections \
    -mgeneral-regs-only \
    -mabi=sysv \
    -mno-red-zone \
    -mcmodel=kernel \

override CPPFLAGS := \
    -I src/ \
    -I deps/limine-protocol/include \
    -I deps/flanterm/src/ \
    -I deps/nanoprintf/ \
    -I deps/ \
    -isystem deps/freestnd-c-hdrs/include \
    -isystem src/libc \
    $(CPPFLAGS) \
    -DLIMINE_API_REVISION=4 \
    -DFLANTERM_FB_BUMP_ALLOC_POOL_SIZE=0x180000 \
    -DFLANTERM_FB_WIDTH_LIMIT=1920 \
    -DFLANTERM_FB_HEIGHT_LIMIT=1080 \
    -MMD \
    -MP \

ifeq ($(ARCH),x86_64)
    override NASMFLAGS := \
        $(patsubst -g,-g -F dwarf,$(NASMFLAGS)) \
        -Wall
endif

ifeq ($(ARCH),x86_64)
    ifeq ($(CC_IS_CLANG),1)
        override CC += \
            -target x86_64-unknown-none-elf
    endif
    override CFLAGS += \
        -m64 \
        -march=x86-64
    override LDFLAGS += \
        -m elf_x86_64
    override NASMFLAGS := \
        -f elf64 \
        $(NASMFLAGS)
endif

override LDFLAGS += \
    -nostdlib \
    --no-pie \
    -static \
    -z max-page-size=0x1000 \
    --gc-sections \
    -T linker_scripts/$(ARCH).lds

override SRCFILES := $(shell find -L src -type f -not -path 'src/arch/*' 2>/dev/null | LC_ALL=C sort)
override SRCFILES += $(shell find -L deps/cc-runtime/src -type f 2>/dev/null | LC_ALL=C sort)
override SRCFILES += $(shell find -L deps/flanterm/src  -type f 2>/dev/null | LC_ALL=C sort)
override SRCFILES += $(shell find -L src/arch/$(ARCH) -type f 2>/dev/null | LC_ALL=C sort)

override CFILES := $(filter %.c,$(SRCFILES))
override ASFILES := $(filter %.S,$(SRCFILES))
ifeq ($(ARCH),x86_64)
override NASMFILES := $(filter %.asm,$(SRCFILES))
endif
override OBJ := $(addprefix obj-$(ARCH)/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o))
ifeq ($(ARCH),x86_64)
override OBJ += $(addprefix obj-$(ARCH)/,$(NASMFILES:.asm=.asm.o))
endif
override HEADER_DEPS := $(addprefix obj-$(ARCH)/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))


.PHONY: all
all: bin-$(ARCH)/$(OUTPUT)

.PHONY: clean
clean:
	rm -rf ./bin-$(ARCH)
	rm -rf ./obj-$(ARCH)
	rm -rf ./kernel.elf

-include $(HEADER_DEPS)

bin-$(ARCH)/$(OUTPUT): GNUmakefile linker_scripts/$(ARCH).lds $(OBJ)
	mkdir -p "$(dir $@)"
	$(LD) $(LDFLAGS) $(OBJ) -o $@

obj-$(ARCH)/%.c.o: %.c GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj-$(ARCH)/%.S.o: %.S GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

ifeq ($(ARCH),x86_64)
obj-$(ARCH)/%.asm.o: %.asm GNUmakefile
	mkdir -p "$(dir $@)"
	nasm $(NASMFLAGS) $< -o $@
endif
