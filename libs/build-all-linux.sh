#!/bin/bash

function build() {
	cd linux
	./jpeg-linux.sh
	./libpng-linux.sh
	./mbedtls-linux.sh
	./curl-linux.sh
	./pixman-linux.sh
	./cairo-linux.sh
	./freetype-linux.sh
	./sqlite-linux.sh
	./glfw-linux.sh
	./hyphen-linux.sh
	cd -
}

function export_dir() {
	rm -f linux/$1/include/png*
	mv linux/$1/include/libpng16/* linux/$1/include
	mv linux/$1/include/freetype2/* linux/$1/include
	mv linux/$1/lib/libpng16.a linux/$1/lib/libpng.a
}

function export_files() {
	export_dir x86_64
}

if [ -z "$1" ]; then
build
export_files
fi

if [ "$1" == "export" ]; then
echo "Export files..."
export_files
fi
