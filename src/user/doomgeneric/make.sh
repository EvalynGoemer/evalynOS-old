#!/usr/bin/env bash

set -e

START_PATH="$PWD"

if [[ "$(basename "$PWD")" != "doomgeneric" ]]; then
    cd ./src/user/doomgeneric || { echo "./src/user/doomgeneric folder dosnt exist? Did you download the repo correctly? Are you in the project root directory?"; exit 1; }
fi

SRC_DIR="$PWD"

DG_BASE_URL="https://github.com/ozkl/doomgeneric/archive/"
DG_COMMIT_HASH="fc601639494e089702a1ada082eb51aaafc03722"
DG_TAR_BALL_URL="${DG_BASE_URL}${DG_COMMIT_HASH}.tar.gz"
DG_TAR_BALL_HASH="6a5879c5f686199f0156ea8abcdaab820350c3a74aff211e81558f8675c8e2d5"

DOOM_SHAREWARE_WAD_URL="https://ia801909.us.archive.org/view_archive.php?archive=/2/items/doom_20230531/doom_dos.ZIP&file=DOOM1.WAD"
DOOM_SHAREWARE_WAD_HASH="1d7d43be501e67d927e415e0b8f3e29c3bf33075e859721816f652a526cac771"

if [ ! -e "${SRC_DIR}/.got-doomgeneric" ]; then
    FILE="${SRC_DIR}/doomgeneric-${DG_COMMIT_HASH}.tar.gz"
    FOLDER="${SRC_DIR}/doomgeneric-${DG_COMMIT_HASH}"
    wget -O "${FILE}" "${DG_TAR_BALL_URL}"
    CALCULATED_HASH=$(sha256sum "$FILE" | awk '{print $1}')
    if [ "${CALCULATED_HASH}" != "${DG_TAR_BALL_HASH}" ]; then
        echo "Hash mismatch for ${FILE}. Expected ${DG_TAR_BALL_HASH} but got ${CALCULATED_HASH}."
        echo "Deleting bad file ${FILE}"
        rm ${FILE}
        exit 1
    fi

    tar -C "${SRC_DIR}" -xf "${FILE}"

    pushd ${FOLDER}
    patch -p1 --forward < ${SRC_DIR}/doomgeneric-evalynos.patch || true
    popd
    touch "${SRC_DIR}/.got-doomgeneric"
else
    echo "doomgeneric already downloaded"
fi

if [ ! -e "${SRC_DIR}/.got-shareware-wad" ]; then
    FILE="${SRC_DIR}/DOOM1.WAD"
    wget -O "${FILE}" "${DOOM_SHAREWARE_WAD_URL}"
    CALCULATED_HASH=$(sha256sum "$FILE" | awk '{print $1}')
    if [ "${CALCULATED_HASH}" != "${DOOM_SHAREWARE_WAD_HASH}" ]; then
        echo "Hash mismatch for ${FILE}. Expected ${DOOM_SHAREWARE_WAD_HASH} but got ${CALCULATED_HASH}."
        echo "Deleting bad file ${FILE}"
        rm ${FILE}
        exit 1
    fi
    mv ${SRC_DIR}/DOOM1.WAD ${SRC_DIR}/../../initramfs
    touch "${SRC_DIR}/.got-shareware-wad"
else
    echo "shareware wad already downloaded"
fi

if [ ! -e "${SRC_DIR}/.compiled-doomgeneric" ]; then
    pushd ${SRC_DIR}/doomgeneric-${DG_COMMIT_HASH}/doomgeneric
    make -f ./Makefile.evalynos clean
    make -f ./Makefile.evalynos -j$(nproc)
    x86_64-evalynos-strip doomgeneric.elf
    mv doomgeneric.elf ../../../../initramfs/doomgeneric.elf
    popd
    touch "${SRC_DIR}/.compiled-doomgeneric"
else
    echo "doomgeneric wad already compiled"
fi

cd "$START_PATH"
