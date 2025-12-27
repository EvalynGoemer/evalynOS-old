set -e

xorriso \
  -as mkisofs \
  -R \
  -r \
  -J \
  -b bios.bin \
  -no-emul-boot \
  -boot-load-size 4 \
  -boot-info-table \
  -hfsplus \
  -apm-block-size 2048 \
  --efi-boot uefi.bin \
  -efi-boot-part \
  --efi-boot-image \
  --protective-msdos-label \
  ./iso \
  -o evalynOS.iso
