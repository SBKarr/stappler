FROM sbkarr/stappler-serenity-dev:latest as core
WORKDIR /serenity/handler
COPY . .
RUN \
	apk add --no-cache build-base binutils-gold \
	&& make APACHE_INCLUDE=/serenity/apache/include STAPPLER_ROOT=/serenity/stappler RELEASE=1 CONF=docker -j4 \
	&& rm -rf lib/host \
	&& rm -rf /serenity/stappler/build/gcc/release/serenity \
	&& apk del build-base binutils-gold

EXPOSE 8080

ENTRYPOINT [ "/serenity/handler/lib/ctrl.sh" ]
