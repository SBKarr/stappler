#!/bin/bash

rm -rf src
mkdir src
cd src

function get_jpeg() {
	wget http://www.ijg.org/files/jpegsrc.v9b.tar.gz
	tar -xzf jpegsrc.v9b.tar.gz
	mv jpeg-9b jpeg
	rm jpegsrc.v9b.tar.gz
}

function get_png() {
	wget http://prdownloads.sourceforge.net/libpng/libpng-1.6.24.tar.gz
	tar -xzf libpng-1.6.24.tar.gz
	rm libpng-1.6.24.tar.gz
	mv libpng-1.6.24 libpng
}

function get_mbedtls() {
	wget https://tls.mbed.org/download/mbedtls-2.3.0-apache.tgz
	tar -xzf mbedtls-2.3.0-apache.tgz
	rm mbedtls-2.3.0-apache.tgz
	mv mbedtls-2.3.0 mbedtls
	sed -i -- 's/mbedtls_time_t/time_t/g' mbedtls/include/mbedtls/ssl.h
}

function get_curl() {
	wget https://curl.haxx.se/download/curl-7.50.3.tar.gz
	tar -xzf curl-7.50.3.tar.gz
	rm curl-7.50.3.tar.gz
	mv curl-7.50.3 curl
}

function get_pixman() {
	wget https://cairographics.org/snapshots/pixman-0.33.6.tar.gz
	tar -xzf pixman-0.33.6.tar.gz
	rm pixman-0.33.6.tar.gz
	mv pixman-0.33.6 pixman
}

function get_cairo() {
	wget https://cairographics.org/snapshots/cairo-1.15.2.tar.xz
	tar -xJf cairo-1.15.2.tar.xz
	rm cairo-1.15.2.tar.xz
	mv cairo-1.15.2 cairo
}

function get_freetype() {
	wget http://ftp.acc.umu.se/mirror/gnu.org/savannah//freetype/freetype-2.7.tar.gz
	tar -xzf freetype-2.7.tar.gz
	rm freetype-2.7.tar.gz
	mv freetype-2.7 freetype
}

function get_sqlite() {
	wget https://www.sqlite.org/2016/sqlite-amalgamation-3140000.zip
	unzip sqlite-amalgamation-3140000.zip -d .
	rm sqlite-amalgamation-3140000.zip
	mv sqlite-amalgamation-3140000 sqlite
}

function get_glfw() {
	wget https://github.com/glfw/glfw/releases/download/3.2/glfw-3.2.zip
	unzip glfw-3.2.zip -d .
	rm glfw-3.2.zip
	mv glfw-3.2 glfw
}

function get_hyphen() {
	git clone https://github.com/SBKarr/hyphen.git
}

function get_cocos2d() {
	git clone https://github.com/SBKarr/stappler-cocos2d-x.git
}

get_jpeg
get_png
get_mbedtls
get_curl
get_pixman
get_cairo
get_freetype
get_sqlite
get_glfw
get_hyphen
get_cocos2d

cd -
