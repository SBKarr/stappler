#!/bin/bash

SAVED_PATH=$PATH

Compile () {

mkdir -p sqlite
cd sqlite

export PATH="/usr/bin/$1-w64-mingw32:$SAVED_PATH"

gcc -c -o sqlite3.o ../../src/sqlite/sqlite3.c
ar rcs libsqlite3.a sqlite3.o

mv -f libsqlite3.a ../$1/lib/
cp -f ../../src/sqlite/*.h ../$1/include/

cd -
rm -rf sqlite

}

Compile x86_64
Compile i686
