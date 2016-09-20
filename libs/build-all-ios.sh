#!/bin/bash

function build() {
	cd ios
	./jpeg-ios.sh
	./libpng-ios.sh
	./curl-ios.sh
	./pixman-ios.sh
	./cairo-ios.sh
	./freetype-ios.sh
	./hyphen-ios.sh
	cd -
}

function export_dir() {
	mv -f ios/$1/include/libpng16/* ios/$1/include
	mv ios/$1/include/freetype2/* ios/$1/include
}

function export_lib() {
	mkdir -p lib
	lipo -create -output lib/$1.a \
		ios/x86_64/lib/$1.a \
		ios/i386/lib/$1.a \
		ios/armv7/lib/$1.a \
		ios/armv7s/lib/$1.a \
		ios/arm64/lib/$1.a
}

function export_files() {
	export_dir i386
	export_dir x86_64
	export_dir armv7
	export_dir armv7s
	export_dir arm64

	export_lib libpng16
	export_lib libjpeg
	export_lib libcurl
	export_lib libfreetype
	export_lib libpixman-1
	export_lib libcairo
	export_lib libhyphen
}

if [ -z "$1" ]; then
build
export_files
fi

if [ "$1" == "export" ]; then
echo "Export files..."
export_files
fi
