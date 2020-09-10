FROM sbkarr/apache-httpd-serenity:latest as core
RUN \
	apk add --no-cache build-base git binutils-gold \
	&& cd /serenity \
	&& git clone --depth=1 https://github.com/SBKarr/stappler.git \
	&& cd stappler \
	&& git submodule update --depth=1 --init \
	&& make APACHE_INCLUDE=/serenity/apache/include RELEASE=1 LDFLAGS=-lexecinfo libserenity -j4 \
	&& cd components/serenity/config \
	&& make RELEASE=1 install \
	&& rm -rf bin/host/gcc \
	&& cd /serenity/stappler \
	&& rm -rf .git libs/.git \
	&& rm -rf libs/android libs/external libs/mac libs/ios libs/win32 \
	&& rm -rf components/stappler components/material components/stappler-cocos2d-x components/xenolith \
	&& rm -rf /serenity/stappler/build/gcc/release/serenity \
	&& rm -rf /serenity/stappler/build/gcc/release/common \
	&& apk del build-base git binutils-gold
