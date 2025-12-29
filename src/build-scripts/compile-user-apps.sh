#!/usr/bin/env bash
# This is a temp hack before the build system is done

set -e

pushd ./src/user/badapple/
./make.sh
popd

pushd ./src/user/doomgeneric/
./make.sh
popd

pushd ./src/user/hello_world/
./make.sh
popd
