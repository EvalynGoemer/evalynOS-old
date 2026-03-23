# EvalynOS
A W.I.P 64 Bit OS Kernel

## Compiling & Running

This kernel uses the [jinx](https://codeberg.org/Mintsuki/jinx) meta build system to compile. You will need all dependancies for such. If you use the [nix](https://nixos.org/) package manager, you can run a developer shell with all necessary dependencies with `nix develop`.

Run `make bootstrap` to build the required things for compiling everything else  
Run `make initramfs` to build all packages for the initramfs and generate it  
Run `make run` to build the ISO and run it in QEMU  
Run `make mkiso` to build the ISO

## Real Hardware
This kernel is designed to be able to run on *REAL* hardware if it meets the following requirements
- x86-64 CPU
- 512MB Memory
- A 1280x720 or higher monitor (1920x1080 recommended)
- A PS/2 keyboard
- A PC speaker

Flash the `evalynOS.iso` file to a usb with a command such as `dd if=./evalynOS.iso of=/dev/{USB_STICK}` and then it can be booted om any system.

## Kernel cmdline args
- `serial=on`: Enables serial on port `0x3F8` on all machines
- `serial=on`: Enables serial on port `0x3F8` when under hypervisor
- `serial=0xXXXX`: Enables serial on the hex I/O port given
- `dbg=on`: Enables extended logging
- `dbg=stub`: Enables the gdb stub server. Requires a working serial port to connect

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
