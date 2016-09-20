#!/bin/sh

SAVED_PATH=$PATH
LIBNAME="freetype"

Compile () {

mkdir -p $LIBNAME
cd $LIBNAME

../../src/$LIBNAME/configure \
	CC="clang" CFLAGS="-Os -fPIC" \
	CPP="clang -E" CPPFLAGS="-I`pwd`/../$1/include" \
	LDFLAGS="-L`pwd`/../$1/lib" \
	PKG_CONFIG_PATH="`pwd`/../$1/lib/pkgconfig" \
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	--prefix=`pwd` \
	--with-bzip2=no \
	--with-zlib=yes \
	--with-png=yes \
	--with-harfbuzz=no \
	--with-pic=yes \
	--enable-static=yes \
	--enable-shared=no

make
make install

cd -
rm -rf $LIBNAME

}

Compile x86_64
