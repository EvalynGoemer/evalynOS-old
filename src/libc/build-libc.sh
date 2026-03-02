#!/usr/bin/env bash
# Made by PCC (@averymt) for evalynOS

set -ex

cd "$(dirname -- "$0")"

BINUTILS_VER=2.45.1
GCC_VER=15.2.0
GMP_VER=6.3.0
MPFR_VER=4.2.2
MPC_VER=1.3.1

PATCHES="../patches"
mkdir -p sysroot toolchain

SYSROOT="$(realpath sysroot)"
TOOLCHAIN="$(realpath toolchain)"
BINUTILS="binutils-${BINUTILS_VER}"
GCC="gcc-${GCC_VER}"
PATH=$PATH:${TOOLCHAIN}/usr/bin

for url in https://ftpmirror.gnu.org/{binutils/${BINUTILS},gcc/${GCC}/${GCC},gmp/gmp-${GMP_VER},mpfr/mpfr-${MPFR_VER},mpc/mpc-${MPC_VER}}.tar.gz; do
    wget -c $url &
done
wait

[ -z "$NOCLEAN" ] && rm -rf ${BINUTILS} ${GCC}

for pkg in ${BINUTILS}.tar.gz ${GCC}.tar.gz gmp-${GMP_VER}.tar.gz mpfr-${MPFR_VER}.tar.gz mpc-${MPC_VER}.tar.gz; do
    tar xzf $pkg &
done
wait

rm -rf gcc-${GCC_VER}/{gmp,mpfr,mpc}
mv gmp-${GMP_VER} ${GCC}/gmp
mv mpfr-${MPFR_VER} ${GCC}/mpfr
mv mpc-${MPC_VER} ${GCC}/mpc

[ -z "$NOCLEAN" ] && {
    patch -d binutils-${BINUTILS_VER} -p1 --forward < ${PATCHES}/binutils-gdb-evalynos.patch
    patch -d gcc-${GCC_VER} -p1 --forward < ${PATCHES}/gcc-evalynos.patch
}

git submodule update --init --recursive mlibc
if git -C mlibc apply --check ../${PATCHES}/mlibc-evalynos.patch; then
    git -C mlibc apply ../${PATCHES}/mlibc-evalynos.patch
fi

rm -rf mlibc/headers-build ${BINUTILS}/build  ${GCC}/build
mkdir -p mlibc/headers-build ${BINUTILS}/build ${GCC}/build

pushd mlibc
meson setup --cross-file=../mlibc-evalynos.cross --prefix=/usr -Dheaders_only=true headers-build
DESTDIR="${SYSROOT}" ninja -C headers-build install
popd

pushd ${BINUTILS}/build
../configure \
    --target=x86_64-evalynos \
    --prefix=/usr \
    --with-sysroot="${SYSROOT}" \
    --disable-werror \
    --enable-default-execstack=no
make -j$(nproc)
DESTDIR="${TOOLCHAIN}" make install
popd

pushd ${GCC}/build
../configure \
    --target=x86_64-evalynos \
    --prefix=/usr \
    --with-sysroot="${SYSROOT}" \
    --enable-languages=c,c++ \
    --enable-threads=posix \
    --disable-multilib \
    --disable-shared \
    --disable-host-shared
make -j$(nproc) all-gcc all-target-libgcc
DESTDIR="${TOOLCHAIN}" make install-gcc install-target-libgcc
popd

pushd mlibc
meson setup --cross-file=../mlibc-evalynos.cross --prefix=/usr -Ddefault_library=static -Dno_headers=true build
DESTDIR="${SYSROOT}" ninja -C build install
popd

if git -C mlibc apply --reverse --check ../${PATCHES}/mlibc-evalynos.patch; then
    git -C mlibc apply --reverse ../${PATCHES}/mlibc-evalynos.patch
fi
