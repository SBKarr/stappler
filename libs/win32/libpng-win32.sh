#!/bin/sh

PNG_FLAGS="--enable-shared=no  --enable-static=yes"
SAVED_PATH=$PATH


Compile () {

mkdir -p libpng
cd libpng

export PATH="/usr/bin/$1-w64-mingw32:$SAVED_PATH"

../../src/libpng/configure --host=$1-w64-mingw32 \
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	CPPFLAGS="-I`pwd`/../$1/include" $PNG_FLAGS \
	LDFLAGS="-L`pwd`/../$1/lib" $PNG_FLAGS \
	--prefix=`pwd`

make
make install

cd -
rm -rf libpng

}

Compile x86_64
Compile i686
