#!/bin/bash
# Android/ARM, armeabi (ARMv5TE soft-float), Android 2.2+ (Froyo)

CFLAGS="-DCAIRO_NO_MUTEX -Os"
ORIGPATH=$PATH
LIBNAME=cairo
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
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	PKG_CONFIG_PATH="`pwd`/../$1/lib/pkgconfig" \
	CPPFLAGS="-I`pwd`/../$1/include -I`pwd`/../$1/include/pixman-1 --sysroot $TOOLCHAIN/sysroot" \
	LDFLAGS="-L`pwd`/../$1/lib" \
	--prefix=`pwd` \
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
	 -disable-silent-rules \
	--disable-some-floating-point \
	--enable-shared=no \
	--enable-static=yes \
	--with-pic=yes
make
make install

cd -
rm -rf $LIBNAME
export PATH=$ORIGPATH

}

Compile	armeabi		9 "" ""
Compile	armeabi-v7a	14 '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16' '-march=armv7-a -Wl,--fix-cortex-a8'
Compile	x86 		14 '' ''
