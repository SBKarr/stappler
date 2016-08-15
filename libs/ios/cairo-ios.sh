#!/bin/sh
CFLAGS="-DCAIRO_NO_MUTEX -Os"
ORIGPATH=$PATH
LIBNAME=cairo
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
CPPFLAGS="-arch $1 -I`pwd`/../$1/include -I`pwd`/../$1/include/pixman-1 -isysroot $4" \
PKG_CONFIG_PATH="`pwd`/../$1/lib/pkgconfig" \
--host=$HOST_VALUE \
--with-sysroot="$4" \
--prefix=`pwd` \
--includedir=`pwd`/../$1/include \
--libdir=`pwd`/../$1/lib \
--enable-xlib=no \
--enable-xlib-xrender=no \
--enable-xcb=no \
--enable-xlib-xcb=no \
--enable-xcb-shm=no \
--enable-qt=no \
--enable-quartz=no \
--enable-quartz-font=no \
--enable-quartz-image=no \
--enable-win32=no \
--enable-win32-font=no \
--enable-skia=no \
--enable-os2=no \
--enable-beos=no \
--enable-drm=no \
--enable-gallium=no \
--enable-png=yes \
--enable-gl=no \
--enable-glesv2=no \
--enable-cogl=no \
--enable-directfb=no \
--enable-vg=no \
--enable-egl=no \
--enable-glx=no \
--enable-wgl=no \
--enable-script=no \
--enable-ft=no \
--enable-fc=no \
--enable-ps=no \
--enable-pdf=no \
--enable-svg=no \
--enable-test-surfaces=no \
--enable-tee=no \
--enable-xml=no \
--enable-pthread=no \
--enable-gobject=no \
--enable-interpreter=no \
--disable-silent-rules \
--disable-some-floating-point \
--enable-shared=no \
--enable-static=yes \
--with-pic=yes \

make
make install

cd -
rm -rf $LIBNAME
}

Compile i386 4.2 $SDK_INCLUDE_SIM $SYSROOT_SIM
Compile x86_64 4.2 $SDK_INCLUDE_SIM $SYSROOT_SIM
Compile armv7 4.2 $SDK_INCLUDE_OS $SYSROOT_OS
Compile armv7s 5.0 $SDK_INCLUDE_OS $SYSROOT_OS
Compile arm64 6.0 $SDK_INCLUDE_OS $SYSROOT_OS
