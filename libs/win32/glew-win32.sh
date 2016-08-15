#!/bin/bash

SAVED_PATH=$PATH

Compile () {

export PATH="/usr/bin/$1-w64-mingw32:$SAVED_PATH"
which gcc

make -C ../src/glew clean
make -C ../src/glew CFLAGS="-O3 -I`pwd`/../../src/glew/include -D_WIN32" CC=gcc

mv -f ../src/glew/lib/libglew32.a $1/lib/
cp -fR ../src/glew/include/* $1/include/

}

Compile x86_64
Compile i686
