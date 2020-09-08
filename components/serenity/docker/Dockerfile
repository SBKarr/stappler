FROM sbkarr/apache-httpd-serenity:latest as core
RUN \
	apk add --no-cache build-base git \
	&& cd /serenity \
	&& git clone --depth=1 https://github.com/SBKarr/stappler.git \
	&& cd stappler \
	&& git submodule update --depth=1 --init \
	&& make APACHE_INCLUDE=/serenity/apache/include RELEASE=1 libserenity -j4 \
	&& rm -rf .git libs/.git \
	&& rm -rf libs/android libs/external libs/mac libs/ios libs/win32 \
	&& rm -rf components/stappler components/material components/stappler-cocos2d-x components/xenolith \
	&& rm -rf /serenity/stappler/build/gcc/release/serenity \
	&& apk del build-base git
