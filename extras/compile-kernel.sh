#!/usr/bin/env bash

set -e

cd "$(dirname -- "$0")"

JINX_DIR="$(realpath ../jinx/)"
KERNEL_DIR="$(realpath ../kernel)"
ISO_DIR="$(realpath ./iso/)"

cd ${KERNEL_DIR}
make -j${nproc}
cp ${KERNEL_DIR}/bin-x86_64/kernel.elf ${ISO_DIR}
