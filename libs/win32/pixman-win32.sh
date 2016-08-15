#!/bin/sh

SAVED_PATH=$PATH

Compile () {

mkdir -p pixman
cd pixman

export PATH="/usr/bin/$1-w64-mingw32:$SAVED_PATH"

../../src/pixman/configure --host=$1-w64-mingw32 \
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	--enable-shared=no \
	--enable-static=yes \
	--with-pic=yes \
	--enable-libpng=yes \
	PKG_CONFIG_PATH="`pwd`/../$1/lib/pkgconfig" \
	CPPFLAGS="-I`pwd`/../$1/include" \
	LDFLAGS="-L`pwd`/../$1/lib" \
	--prefix=`pwd`

make
make install

cd -
rm -rf pixman

}

Compile x86_64
Compile i686
