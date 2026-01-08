set -e

xorriso \
  -as mkisofs \
  -V "EvalynOS"\
  -R \
  -r \
  -J \
  -b limine-bios-cd.bin \
  -no-emul-boot \
  -boot-load-size 4 \
  -boot-info-table \
  -hfsplus \
  -apm-block-size 2048 \
  --efi-boot limine-uefi-cd.bin \
  -efi-boot-part \
  --efi-boot-image \
  --protective-msdos-label \
  ./src/generated/iso \
  -o evalynOS.iso

limine bios-install evalynOS.iso
