#!/bin/bash
# Android/ARM, armeabi (ARMv5TE soft-float), Android 2.2+ (Froyo)

CFLAGS="-Os"
ORIGPATH=$PATH
LIBNAME=mbedtls
ROOT=`pwd`

Compile () {

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
NDKLDFLAGS=$4

rm -f ../src/mbedtls/library/libmbed*

make -C ../src/mbedtls clean
make -C ../src/mbedtls CC=$NDKP-clang AR=$NDKP-ar CFLAGS="$NDKF $NDKARCH" lib

mkdir -p `pwd`/$1/include
mkdir -p `pwd`/$1/lib

cp -r ../src/mbedtls/include/mbedtls `pwd`/$1/include
cp -RP ../src/mbedtls/library/libmbed* `pwd`/$1/lib

export PATH=$ORIGPATH

}

Compile	armeabi		9 "" ""
Compile	armeabi-v7a	14 '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16' '-march=armv7-a -Wl,--fix-cortex-a8'
Compile	x86 		14 '' ''

