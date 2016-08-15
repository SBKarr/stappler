#!/bin/bash
# Android/ARM, armeabi (ARMv5TE soft-float), Android 2.2+ (Froyo)

CFLAGS="-Os -I$NDK/sources/android/cpufeatures"
ORIGPATH=$PATH
LIBNAME=pixman
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
NDKLDFLAGS=$4
../../src/$LIBNAME/configure $CONFFLAGS \
	CC=$NDKP-clang CFLAGS="$NDKF $NDKARCH" \
	LD=$NDKP-ld LDFLAGS="$NDKLDFLAGS" \
	--host=$TARGET --with-sysroot="$TOOLCHAIN/sysroot"\
	--disable-arm-iwmmxt \
	--enable-shared=no \
	--enable-static=yes \
	--with-pic=yes \
	--enable-libpng=yes \
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	CPPFLAGS="-I`pwd`/../$1/include --sysroot $TOOLCHAIN/sysroot" \
	LDFLAGS="-L`pwd`/../$1/lib" \
	PKG_CONFIG_PATH="`pwd`/../$1/lib/pkgconfig" \
	--prefix=`pwd`
make
make install

cd -
rm -rf $LIBNAME
export PATH=$ORIGPATH

}

Compile	armeabi		9 "" ""
Compile	armeabi-v7a	14 '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16' '-march=armv7-a -Wl,--fix-cortex-a8'
Compile	x86 		14 '' ''
