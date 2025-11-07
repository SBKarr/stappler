
define newline


endef

noop=
space = $(noop) $(noop)
tab = $(noop)	$(noop)

BUILD_WORKDIR = $(patsubst %/,%,$(dir $(realpath $(firstword $(MAKEFILE_LIST)))))
