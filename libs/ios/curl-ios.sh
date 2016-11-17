#!/bin/sh

CFLAGS="-pipe -Os -gdwarf-2 -Werror=partial-availability"
ORIGPATH=$PATH
LIBNAME=curl
ROOT=`pwd`

XCODE_BIN_PATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"
export PATH=$XCODE_BIN_PATH:$PATH

SDK_INCLUDE_SIM="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk/usr/include"
SDK_INCLUDE_OS="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/usr/include"

SYSROOT_SIM="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk"
SYSROOT_OS="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"

Compile () {
mkdir -p $LIBNAME
cd $LIBNAME

export IPHONEOS_DEPLOYMENT_TARGET="$2"
HOST_VALUE=$1-apple-darwin
if [ "$1" == "arm64" ]; then
HOST_VALUE=arm-apple-darwin
fi

../../src/$LIBNAME/configure \
	CC=$XCODE_BIN_PATH/clang \
	LD=$XCODE_BIN_PATH/ld \
	CPP="$XCODE_BIN_PATH/clang -E" \
	CFLAGS="$CFLAGS -arch $1 -isysroot $4  -miphoneos-version-min=$2" \
	LDFLAGS="-arch $1 -isysroot $4 -miphoneos-version-min=$2 -L`pwd`/../$1/lib" \
	CPPFLAGS="-arch $1 -I`pwd`/../$1/include -isysroot $4" \
	--host=$HOST_VALUE \
	--with-sysroot="$4" \
	--prefix=`pwd` \
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	--disable-shared --enable-static --enable-optimize --enable-symbol-hiding --enable-ftp --disable-ldap --enable-file --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-smb --disable-tftp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-manual --with-darwinssl --enable-threaded-resolver

make
make install

cd -
rm -rf $LIBNAME
}

Compile i386 6.0 $SDK_INCLUDE_SIM $SYSROOT_SIM
Compile x86_64 6.0 $SDK_INCLUDE_SIM $SYSROOT_SIM
Compile armv7 6.0 $SDK_INCLUDE_OS $SYSROOT_OS
Compile armv7s 6.0 $SDK_INCLUDE_OS $SYSROOT_OS
Compile arm64 6.0 $SDK_INCLUDE_OS $SYSROOT_OS
