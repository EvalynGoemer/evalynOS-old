#!/usr/bin/env bash

set -e

cd "$(dirname -- "$0")"

JINX_DIR="$(realpath ../jinx/)"
SRC_DIR="$(realpath ./initramfs)"
ISO_DIR="$(realpath ./iso/)"

PACKAGES=(
    doomgeneric
    badapple
    helloworld
    bash
)

cd ${JINX_DIR}
./jinx build     "${PACKAGES[@]}"
./jinx reinstall "$SRC_DIR" "${PACKAGES[@]}"
cd ${SRC_DIR}
strip ./usr/bin/bash
mv ./usr/bin/bash ./bash.elf
rm -rf ./usr
rm -rf ./var
cd ${ISO_DIR}
rm -rf ./var

tar --sort=name \
    --mtime='UTC 2026-01-01' \
    --owner=0 --group=0 --numeric-owner \
    -C "${SRC_DIR}" \
    -cf - . | gzip -9 > "${ISO_DIR}/initramfs.tar.gz"

