#!/usr/bin/env bash

set -ex

START_PATH="$PWD"

if [[ "$(basename "$PWD")" != "libc" ]]; then
    cd ./src/libc || { echo "./src/libc folder dosnt exist? Did you download the repo correctly? Are you in the project root directory?"; exit 1; }
fi

BINUTILS_VER=2.45.1
GCC_VER=15.2.0

SRC_DIR="$PWD"
PATCHES_DIR="$PWD/../patches"
mkdir -p ${SRC_DIR}/sysroot
mkdir -p ${SRC_DIR}/toolchain

SYSROOT_DIR="${SRC_DIR}/sysroot"
TOOLCHAIN_DIR="${SRC_DIR}/toolchain"
MLIBC_DIR="${SRC_DIR}/mlibc"
BINUTILS_DIR="${SRC_DIR}/binutils-${BINUTILS_VER}"
GCC_DIR="${SRC_DIR}/gcc-${GCC_VER}"
PATH=$PATH:${TOOLCHAIN_DIR}/usr/bin

CFLAGS_FOR_TARGET="-march=x86-64 -mabi-sysv"
CXXFLAGS_FOR_TARGET="-march=x86-64 -mabi-sysv"

git submodule init ./mlibc
pushd ${MLIBC_DIR}
if git apply --check ${PATCHES_DIR}/mlibc-evalynos.patch; then
    git apply ${PATCHES_DIR}/mlibc-evalynos.patch
fi
popd

if [ ! -e "${SRC_DIR}/.got-binutils" ]; then
    wget -O "${SRC_DIR}/binutils-${BINUTILS_VER}.tar.xz" https://ftpmirror.gnu.org/binutils/binutils-${BINUTILS_VER}.tar.xz
    tar  -C "${SRC_DIR}" -xf "${SRC_DIR}/binutils-${BINUTILS_VER}.tar.xz"

    pushd ${BINUTILS_DIR}
    patch -p1 --forward < ${PATCHES_DIR}/binutils-gdb-evalynos.patch || true
    popd

    touch ${SRC_DIR}/.got-binutils
else
    echo "binutils already downloaded"
fi

if [ ! -e "${SRC_DIR}/.got-gcc" ]; then
    wget -O "${SRC_DIR}/gcc-${GCC_VER}.tar.xz" https://ftpmirror.gnu.org/gnu/gcc/gcc-${GCC_VER}/gcc-${GCC_VER}.tar.xz
    tar  -C "${SRC_DIR}" -xf "${SRC_DIR}/gcc-${GCC_VER}.tar.xz"

    pushd ${GCC_DIR}
    patch -p1 --forward < ${PATCHES_DIR}/gcc-evalynos.patch || true
    popd

    touch ${SRC_DIR}/.got-gcc
else
    echo "gcc already downloaded"
fi

if [ -d ${MLIBC_DIR}/headers-build ]; then
    if [ "$(ls -A ${MLIBC_DIR}/headers-build)" ]; then
        rm -rf ${MLIBC_DIR}/headers-build
        mkdir -p ${MLIBC_DIR}/headers-build
    fi
fi

if [ -d ${BINUTILS_DIR}/build ]; then
    if [ "$(ls -A ${BINUTILS_DIR}/build)" ]; then
        rm -rf ${BINUTILS_DIR}/build
        mkdir -p ${BINUTILS_DIR}/build
    fi
else
    mkdir -p ${BINUTILS_DIR}/build
fi

if [ -d ${GCC_DIR}/build ]; then
    if [ "$(ls -A ${GCC_DIR}/build)" ]; then
        rm -rf ${GCC_DIR}/build
        mkdir -p ${GCC_DIR}/build
    fi
else
    mkdir -p ${GCC_DIR}/build
fi

if [ ! -e "${SRC_DIR}/.built-mlibc-headers" ]; then
    pushd ${MLIBC_DIR}
    meson setup --cross-file=${SRC_DIR}/mlibc-evalynos.cross --prefix=/usr -Dheaders_only=true headers-build
    DESTDIR="${SYSROOT_DIR}" ninja -C headers-build install
    popd
    touch ${SRC_DIR}/.built-mlibc-headers
else
    echo "mlibc headers already built"
fi

if [ ! -e "${SRC_DIR}/.built-binutils" ]; then
    pushd ${BINUTILS_DIR}/build
    ${BINUTILS_DIR}/configure           \
        --target=x86_64-evalynos        \
        --prefix=/usr                   \
        --with-sysroot="${SYSROOT_DIR}" \
        --disable-werror                \
        --enable-default-execstack=no
    make -j$(nproc)
    DESTDIR="${TOOLCHAIN_DIR}" make install
    popd
    touch ${SRC_DIR}/.built-binutils
else
    echo "binutils already built"
fi

if [ ! -e "${SRC_DIR}/.built-gcc" ]; then
    pushd ${GCC_DIR}/build
    ${GCC_DIR}/configure                           \
        --target=x86_64-evalynos                   \
        --prefix=/usr                              \
        --with-sysroot="${SYSROOT_DIR}"            \
        --enable-languages=c,c++                   \
        --enable-threads=posix                     \
        --disable-multilib                         \
        --enable-shared                            \
        --enable-host-shared
    make -j$(nproc) all-gcc all-target-libgcc
    DESTDIR="${TOOLCHAIN_DIR}" make install-gcc install-target-libgcc
    popd
    touch ${SRC_DIR}/.built-gcc
else
    echo "gcc already built"
fi

if [ ! -e "${SRC_DIR}/.built-mlibc" ]; then
    pushd ${MLIBC_DIR}
    meson setup --cross-file=${SRC_DIR}/mlibc-evalynos.cross --prefix=/usr -Ddefault_library=static -Dno_headers=true build
    DESTDIR=${SYSROOT_DIR} ninja -C build install
    popd
    touch ${SRC_DIR}/.built-mlibc
else
    echo "mlibc already built"
fi

pushd ${MLIBC_DIR}
if git apply --reverse --check ${PATCHES_DIR}/mlibc-evalynos.patch; then
    git apply --reverse ${PATCHES_DIR}/mlibc-evalynos.patch
fi
popd

cd "$START_PATH"
