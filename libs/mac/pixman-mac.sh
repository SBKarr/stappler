#!/bin/sh

SAVED_PATH=$PATH
LIBNAME="pixman"

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
	--enable-shared=no \
	--enable-static=yes \
	--with-pic=yes \
	--enable-libpng=yes

make
make install

cd -
rm -rf $LIBNAME

}

Compile x86_64
