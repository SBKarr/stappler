#!/bin/sh

CFLAGS="-Os"
ORIGPATH=$PATH
LIBNAME=freetype
ROOT=`pwd`

XCODE_BIN_PATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"
#export PATH=$XCODE_BIN_PATH:$PATH

SDK_INCLUDE_SIM="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk/usr/include"
SDK_INCLUDE_OS="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/usr/include"

SYSROOT_SIM="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk"
SYSROOT_OS="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"

Compile () {
mkdir -p $LIBNAME
cd $LIBNAME

#export IPHONEOS_DEPLOYMENT_TARGET="$2"
HOST_VALUE=$5

export CCexe=gcc

../../src/$LIBNAME/configure \
	CC=$XCODE_BIN_PATH/clang \
	LD=$XCODE_BIN_PATH/ld \
	CPP="$XCODE_BIN_PATH/clang -E" \
	CFLAGS="$CFLAGS -arch $1 -isysroot $4  -miphoneos-version-min=$2" \
	LDFLAGS="-arch $1 -isysroot $4 -miphoneos-version-min=$2 -L`pwd`/../$1/lib" \
	CPPFLAGS="-arch $1 -I`pwd`/../$1/include -isysroot $4" \
	PKG_CONFIG_PATH="`pwd`/../$1/lib/pkgconfig" \
	--host=$HOST_VALUE \
	--with-sysroot="$4" \
	--prefix=`pwd` \
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	--with-bzip2=no --with-zlib=yes --with-png=yes --enable-static=yes --enable-shared=no

make CCexe=gcc
make CCexe=gcc install

cd -
rm -rf $LIBNAME
}

Compile i386 6.0 $SDK_INCLUDE_SIM $SYSROOT_SIM i386-apple-darwin
Compile x86_64 6.0 $SDK_INCLUDE_SIM $SYSROOT_SIM x86_64-apple-darwin
Compile armv7 6.0 $SDK_INCLUDE_OS $SYSROOT_OS armv7-apple-darwin
Compile armv7s 6.0 $SDK_INCLUDE_OS $SYSROOT_OS armv7s-apple-darwin
Compile arm64 6.0 $SDK_INCLUDE_OS $SYSROOT_OS arm-apple-darwin
