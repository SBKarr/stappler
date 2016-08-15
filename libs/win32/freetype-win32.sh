#!/bin/sh

SAVED_PATH=$PATH

Compile () {

mkdir -p freetype
cd freetype

export PATH="/usr/bin/$1-w64-mingw32:$SAVED_PATH"

../../src/freetype/configure --host=$1-w64-mingw32 \
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	--with-bzip2=no \
	--with-zlib=yes \
	--with-png=yes \
	--with-harfbuzz=no \
	--with-pic=yes \
	--enable-static=yes \
	--enable-shared=no \
	PKG_CONFIG_PATH="`pwd`/../$1/lib/pkgconfig" \
	CPPFLAGS="-I`pwd`/../$1/include" \
	LDFLAGS="-L`pwd`/../$1/lib" \
	--prefix=`pwd`

make

echo "Try to replace apinames tools with Cygwin alternative"
rm apinames.exe
/usr/bin/gcc  -o apinames.exe ../../src/freetype/src/tools/apinames.c

make
make install

cd -
rm -rf freetype

}

Compile x86_64
Compile i686
