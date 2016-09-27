#!/bin/bash

function build() {
	cd mac
	./jpeg-mac.sh
	./libpng-mac.sh
	./curl-mac.sh
	./pixman-mac.sh
	./cairo-mac.sh
	./freetype-mac.sh
	./sqlite-mac.sh
	./hyphen-mac.sh
	cd -
}

function export_dir() {
	cp -f mac/$1/include/libpng16/* mac/$1/include
	mv mac/$1/include/freetype2/* mac/$1/include
	mv mac/$1/lib/libpng16.a mac/$1/lib/libpng.a
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
