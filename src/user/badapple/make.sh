x86_64-evalynos-gcc -O2 badapple.c -o badapple.elf
x86_64-evalynos-strip badapple.elf

cp ./badapple.elf ../../initramfs
cd ../../../
