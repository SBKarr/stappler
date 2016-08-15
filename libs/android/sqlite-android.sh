#!/bin/bash

CFLAGS="-Os"
ORIGPATH=$PATH
LIBNAME=sqlite
ROOT=`pwd`

Compile () {

mkdir -p $LIBNAME
cd $LIBNAME

ARCH=$1
NDKABI=$2
TARGET=arm-linux-androideabi

if [ "$1" == "x86" ]; then
TARGET=i686-linux-android
fi

TOOLCHAIN=$ROOT/toolchains/$1
export PATH=$TOOLCHAIN/bin:$PATH
NDKP=$TOOLCHAIN/bin/$TARGET
NDKF="$CFLAGS --sysroot $TOOLCHAIN/sysroot"
NDKARCH=$3

$NDKP-clang $CFLAGS --sysroot "$TOOLCHAIN/sysroot" $NDKARCH -c -o sqlite3.o ../../src/sqlite/sqlite3.c
$NDKP-ar rcs libsqlite3.a sqlite3.o

rm -f ../$1/lib/libsqlite3.a
rm -f ../$1/include/sqlite3.h

mv -f libsqlite3.a ../$1/lib/
cp -f ../../src/sqlite/*.h ../$1/include/

cd -
rm -rf $LIBNAME
export PATH=$ORIGPATH

}

Compile	armeabi		9 "" ""
Compile	armeabi-v7a	14 '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16' '-march=armv7-a -Wl,--fix-cortex-a8'
Compile	x86 		14 '' ''
