x86_64-evalynos-gcc hello_world.c -o hello_world.elf
x86_64-evalynos-strip hello_world.elf

cp ./hello_world.elf ../../initramfs
cd ../../../
