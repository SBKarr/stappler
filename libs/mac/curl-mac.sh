#!/bin/bash

SAVED_PATH=$PATH
LIBNAME="curl"

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
	--disable-rtmp \
	--enable-ipv6 \
	--disable-versioned-symbols \
	--disable-verbose  \
	--disable-sspi \
	--disable-ntlm-wb  \
	--disable-unix-sockets \
	--enable-cookies \
	--enable-shared=no \
	--enable-static=yes \
	--with-zlib \
	--with-darwinssl \
	--enable-threaded-resolver \
	--without-ca-path \
	--without-ca-fallback \
	--without-ca-bundle \
	--without-libmetalink \
	--without-libssh2 \
	--without-libidn \
	--without-ssl \
	--without-librtmp

make
make install

cd -
rm -rf $LIBNAME

}

Compile x86_64
