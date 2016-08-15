#!/bin/bash
# Android/ARM, armeabi (ARMv5TE soft-float), Android 2.2+ (Froyo)

CFLAGS="-Os"
ORIGPATH=$PATH
LIBNAME=curl
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
../../src/$LIBNAME/configure \
	CC=$NDKP-clang CFLAGS="$NDKF $NDKARCH" \
	LD=$NDKP-ld LDFLAGS="$NDKLDFLAGS" \
	AR=$NDKP-ar \
	--host=$TARGET --with-sysroot="$TOOLCHAIN/sysroot" \
	CPPFLAGS="-I`pwd`/../$1/include" \
	LDFLAGS="-L`pwd`/../$1/lib" \
	--disable-optimize \
	--disable-warnings \
	--disable-curldebug \
	--enable-symbol-hiding \
	--disable-rt \
	--disable-largefile \
	--enable-http \
	--enable-ftp \
	--enable-file \
	--disable-ldap \
	--disable-ldaps \
	--disable-rtsp \
	--disable-proxy \
	--disable-dict \
	--disable-telnet \
	--disable-tftp \
	--disable-pop3 \
	--disable-imap \
	--disable-smb \
	--disable-smtp \
	--disable-gopher \
	--disable-manual \
	--enable-ipv6 \
	--disable-versioned-symbols \
	--disable-verbose  \
	--disable-sspi \
	--disable-ntlm-wb  \
	--disable-unix-sockets \
	--enable-cookies \
	--enable-shared=no \
	--enable-static=yes \
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	--prefix=`pwd` \
	--with-zlib \
	--with-mbedtls \
	--without-ca-path \
	--without-ca-fallback \
	--without-ca-bundle \
	--without-libmetalink \
	--without-libssh2
make
make install

cd -
rm -rf $LIBNAME
export PATH=$ORIGPATH

}

Compile	armeabi		9 "" ""
Compile	armeabi-v7a	14 '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16' '-march=armv7-a -Wl,--fix-cortex-a8'
Compile	x86 		14 '' ''
