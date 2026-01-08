# EvalynOS
A W.I.P 64 Bit OS Kernel

## Compiling & Running

You will need to install the following arch packages to compile (Non inclusive, More testing needs to be done)
- base-devel
- python3
- qemu-desktop
- edk2-ovmf
- limine
- mason
- ninja
- Dependancies for GCC & Binutils

Run the script in `/src/libc/build-libc.sh` to build libc

Delete the generated dot files to have the script recompile things if needed.

Add `/src/libc/toolchain/usr/bin/` to your path aswell for things to compile right

Compile with `make`

Run and compile with `make run`

Run and compile and generate symbols (Compiles twice) with `make debug`

## Compile Flags
- `-DMUTE_KERNEL_PANIC`: Disables the 1khz tone played on a kernel panic for when doing active work on the kernel

Add these flags to the `GNUmakefile` `CFLAGS` section

## Real Hardware
This kernel is designed to be able to run on *REAL* hardware if it meets the following requirements
- x86-64 CPU
- 512MB Memory
- A 1280x720 or higher monitor (1920x1080 recommended)
- A PS/2 keyboard
- A PC speaker

Flash the `evalynOS.iso` file to a usb with a command such as `dd if=./evalynOS.iso of=/dev/{USB_STICK}` and then it can be booted om any system.

## Kernel Mode Shell Commands
- `TEST`: Prints a test message
- `CREDITS`: Prints credits.txt
- `CLEAR`: Clears the terminal (ALIAS: `CLS`)
- `MMAP`: Prints the memory map provided by the bootloader
- `SMASH`: Smashes the stack to test stack smashing protection
- `PANIC`: Causes a kernel panic
- `FAULT`: Causes a Page Fault to panic with extended info
- `BADAPPLE`: Plays the Bad Apple demo inside userspace
- `DOOM`: Starts doom in userspace
