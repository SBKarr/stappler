FROM alpine:latest as core
RUN \
	apk add build-base git pcre-dev openssl-dev postgresql-dev expat-dev zlib-dev libexecinfo-dev \
	&& mkdir serenity \
	&& cd serenity \
	&& git clone https://github.com/SBKarr/apache-httpd-serenity.git \
	&& cd apache-httpd-serenity \
	&& ./configure-alpine.sh /serenity/apache \
	&& make -j4 \
	&& make install \
	&& cd .. \
	&& ln -s /usr/include/libpq-fe.h /usr/include/postgresql/libpq-fe.h \
	&& git clone https://github.com/SBKarr/stappler.git \
	&& cd stappler \
	&& git submodule update --init \
	&& make APACHE_INCLUDE=/serenity/apache/include RELEASE=1 libserenity -j4
