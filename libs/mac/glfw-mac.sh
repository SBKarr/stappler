#!/bin/bash

SAVED_PATH=$PATH

Compile () {

mkdir -p glfw
cd glfw

export CC="clang"
export CPP="clang -E"
export CFLAGS="-Os -fPIC"

cmake ../../src/glfw

make CC="clang" CFLAGS="-Os -fPIC" glfw
cp -f src/libglfw3.a ../$1/lib/
cp -f ../../src/glfw/include/GLFW/* ../$1/include/

cd -
rm -rf glfw

}

Compile x86_64
