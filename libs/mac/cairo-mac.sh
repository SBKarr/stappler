#!/bin/sh

SAVED_PATH=$PATH
LIBNAME="cairo"

Compile () {

mkdir -p $LIBNAME
cd $LIBNAME

../../src/$LIBNAME/configure \
	CC="clang" CFLAGS="-Os -DCAIRO_NO_MUTEX -fPIC" \
	CPP="clang -E" CPPFLAGS="-I`pwd`/../$1/include -I`pwd`/../$1/include/pixman-1" \
	LDFLAGS="-L`pwd`/../$1/lib" \
	PKG_CONFIG_PATH="`pwd`/../$1/lib/pkgconfig" \
	--includedir=`pwd`/../$1/include \
	--libdir=`pwd`/../$1/lib \
	--prefix=`pwd` \
	--enable-xlib=no \
	--enable-xlib-xrender=no \
	--enable-xcb=no \
	--enable-xlib-xcb=no \
	--enable-xcb-shm=no \
	--enable-qt=no \
	--enable-quartz=no \
	--enable-quartz-font=no \
	--enable-quartz-image=no \
	--enable-win32=no \
	--enable-win32-font=no \
	--enable-skia=no \
	--enable-os2=no \
	--enable-beos=no \
	--enable-drm=no \
	--enable-gallium=no \
	--enable-png=yes \
	--enable-gl=no \
	--enable-glesv2=no \
	--enable-cogl=no \
	--enable-directfb=no \
	--enable-vg=no \
	--enable-egl=no \
	--enable-glx=no \
	--enable-wgl=no \
	--enable-script=no \
	--enable-ft=no \
	--enable-fc=no \
	--enable-ps=no \
	--enable-pdf=no \
	--enable-svg=no \
	--enable-test-surfaces=no \
	--enable-tee=no \
	--enable-xml=no \
	--enable-pthread=no \
	--enable-gobject=no \
	--enable-interpreter=no \
	 -disable-silent-rules \
	--disable-some-floating-point \
	--enable-shared=no \
	--enable-static=yes \
	--with-pic=yes

make
make install

cd -
rm -rf $LIBNAME

}

Compile x86_64
