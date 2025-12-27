#!/usr/bin/env bash
# This is a temp hack before the build system is done

pushd ./src/user/badapple/
./make.sh
popd

pushd ./src/user/hello_world/
./make.sh
popd
