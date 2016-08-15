#!/bin/sh

SAVED_PATH=$PATH

Compile () {

cd ../src/zlib


export PATH="/usr/bin/$1-w64-mingw32:$SAVED_PATH"

make clean
./configure --prefix=`pwd` \
	--static \
	--includedir=`pwd`/../win32/$1/include \
	--libdir=`pwd`/../win32/$1/lib

make
make install

cd -

}

Compile x86_64
Compile i686
