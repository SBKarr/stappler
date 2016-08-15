#!/bin/bash

SAVED_PATH=$PATH

Compile () {

mkdir -p curl
cd curl

export PATH="/usr/bin/$1-w64-mingw32:$SAVED_PATH"

../../src/curl/configure --host=$1-w64-mingw32 \
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
rm -rf curl

}

Compile x86_64
Compile i686

