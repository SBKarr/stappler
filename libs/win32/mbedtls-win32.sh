#!/bin/bash

SAVED_PATH=$PATH

Compile () {

export PATH="/usr/bin/$1-w64-mingw32:$SAVED_PATH"
rm -f library/libmbed*

make -C ../src/mbedtls clean
make -C ../src/mbedtls CFLAGS="-O3" CC=gcc lib

cp -r ../src/mbedtls/include/mbedtls `pwd`/$1/include
cp -RP ../src/mbedtls/library/libmbed* `pwd`/$1/lib

}

Compile x86_64
Compile i686
