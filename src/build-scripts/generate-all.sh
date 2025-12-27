#!/usr/bin/env bash

mkdir -p ./iso

./src/build-scripts/generate-symbols.py
./src/build-scripts/compile-user-apps.sh
./src/build-scripts/generate-initramfs.sh
./src/build-scripts/generate-iso.sh
