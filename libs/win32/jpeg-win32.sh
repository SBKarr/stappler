#!/bin/sh

JPEG_FLAGS="--enable-shared=no  --enable-static=yes"
SAVED_PATH=$PATH

Compile () {

mkdir -p jpeg
cd jpeg

export PATH="/usr/bin/$1-w64-mingw32:$SAVED_PATH"

../../src/jpeg/configure --host=$1-w64-mingw32 \
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	CPPFLAGS="-I`pwd`/../$1/include" \
	LDFLAGS="-L`pwd`/../$1/lib" \
	$JPEG_FLAGS \
	--prefix=`pwd`

make
make install

cd -
rm -rf jpeg

}

Compile x86_64
Compile i686
