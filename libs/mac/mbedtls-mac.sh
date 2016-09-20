#!/bin/bash

SAVED_PATH=$PATH

Compile () {

rm -f library/libmbed*

make -C ../src/mbedtls clean
make -C ../src/mbedtls CFLAGS="-Os -fPIC" CC=clang lib

cp -r ../src/mbedtls/include/mbedtls `pwd`/$1/include
cp -RP ../src/mbedtls/library/libmbed* `pwd`/$1/lib

}

Compile x86_64
