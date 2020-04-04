/*
  zip_source_compress.c -- (de)compression routines
  Copyright (C) 2017-2019 Dieter Baron and Thomas Klausner

  This file is part of libzip, a library to manipulate ZIP archives.
  The authors can be contacted at <libzip@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The names of the authors may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "libzip_impl.h"

static zip_source_t *
zip_source_layered_create(zip_source_t *src, zip_source_layered_callback cb, void *ud, zip_error_t *error) {
    zip_source_t *zs;

    if ((zs = _zip_source_new(error)) == NULL)
	return NULL;

    zip_source_keep(src);
    zs->src = src;
    zs->cb.l = cb;
    zs->ud = ud;

    zs->supports = cb(src, ud, NULL, 0, ZIP_SOURCE_SUPPORTS);
    if (zs->supports < 0) {
	zs->supports = ZIP_SOURCE_SUPPORTS_READABLE;
    }

    return zs;
}

static zip_source_t *
zip_source_layered(zip_t *za, zip_source_t *src, zip_source_layered_callback cb, void *ud) {
	if (za == NULL)
		return NULL;

	return zip_source_layered_create(src, cb, ud, &za->error);
}


struct context {
    zip_error_t error;

    bool end_of_input;
    bool end_of_stream;
    bool can_store;
    bool is_stored; /* only valid if end_of_stream is true */
    bool compress;
    zip_int32_t method;

    zip_uint64_t size;
    zip_int64_t first_read;
    zip_uint8_t buffer[BUFSIZE];

    zip_compression_algorithm_t *algorithm;
    void *ud;
};


struct implementation {
    zip_uint16_t method;
    zip_compression_algorithm_t *compress;
    zip_compression_algorithm_t *decompress;
};

static struct implementation implementations[] = {
    {ZIP_CM_DEFLATE, &zip_algorithm_deflate_compress, &zip_algorithm_deflate_decompress},
};

static size_t implementations_size = sizeof(implementations) / sizeof(implementations[0]);

static zip_source_t *compression_source_new(zip_t *za, zip_source_t *src, zip_int32_t method, bool compress, int compression_flags);
static zip_int64_t compress_callback(zip_source_t *, void *, void *, zip_uint64_t, zip_source_cmd_t);
static void context_free(struct context *ctx);
static struct context *context_new(zip_int32_t method, bool compress, int compression_flags, zip_compression_algorithm_t *algorithm);
static zip_int64_t compress_read(zip_source_t *, struct context *, void *, zip_uint64_t);

static zip_compression_algorithm_t *
get_algorithm(zip_int32_t method, bool compress) {
	size_t i;
	zip_uint16_t real_method = ZIP_CM_ACTUAL(method);

	for (i = 0; i < implementations_size; i++) {
		if (implementations[i].method == real_method) {
			if (compress) {
				return implementations[i].compress;
			} else {
				return implementations[i].decompress;
			}
		}
	}

	return NULL;
}

ZIP_EXTERN int
zip_compression_method_supported(zip_int32_t method, int compress) {
	if (method == ZIP_CM_STORE) {
		return 1;
	}
	return get_algorithm(method, compress) != NULL;
}

static zip_source_t *
zip_source_compress(zip_t *za, zip_source_t *src, zip_int32_t method, int compression_flags) {
    return compression_source_new(za, src, method, true, compression_flags);
}

static zip_source_t *
zip_source_decompress(zip_t *za, zip_source_t *src, zip_int32_t method) {
    return compression_source_new(za, src, method, false, 0);
}


static zip_source_t *
compression_source_new(zip_t *za, zip_source_t *src, zip_int32_t method, bool compress, int compression_flags) {
	struct context *ctx;
	zip_source_t *s2;
	zip_compression_algorithm_t *algorithm = NULL;

	if (src == NULL) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return NULL;
	}

	if ((algorithm = get_algorithm(method, compress)) == NULL) {
		zip_error_set(&za->error, ZIP_ER_COMPNOTSUPP, 0);
		return NULL;
	}

	if ((ctx = context_new(method, compress, compression_flags, algorithm)) == NULL) {
		zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
		return NULL;
	}

	if ((s2 = zip_source_layered(za, src, compress_callback, ctx)) == NULL) {
		context_free(ctx);
		return NULL;
	}

	return s2;
}


static struct context *
context_new(zip_int32_t method, bool compress, int compression_flags, zip_compression_algorithm_t *algorithm) {
	struct context *ctx;

	if ((ctx = (struct context*) malloc(sizeof(*ctx))) == NULL) {
		return NULL;
	}
	zip_error_init(&ctx->error);
	ctx->can_store = compress ? ZIP_CM_IS_DEFAULT(method) : false;
	ctx->algorithm = algorithm;
	ctx->method = method;
	ctx->compress = compress;
	ctx->end_of_input = false;
	ctx->end_of_stream = false;
	ctx->is_stored = false;

	if ((ctx->ud = ctx->algorithm->allocate(ZIP_CM_ACTUAL(method), compression_flags, &ctx->error)) == NULL) {
		zip_error_fini(&ctx->error);
		free(ctx);
		return NULL;
	}

	return ctx;
}

static void
context_free(struct context *ctx) {
	if (ctx == NULL) {
		return;
	}

	ctx->algorithm->deallocate(ctx->ud);
	zip_error_fini(&ctx->error);

	free(ctx);
}

static zip_int64_t
compress_read(zip_source_t *src, struct context *ctx, void *data, zip_uint64_t len) {
	zip_compression_status_t ret;
	bool end;
	zip_int64_t n;
	zip_uint64_t out_offset;
	zip_uint64_t out_len;

	if (zip_error_code_zip(&ctx->error) != ZIP_ER_OK) {
		return -1;
	}

	if (len == 0 || ctx->end_of_stream) {
		return 0;
	}

	out_offset = 0;

	end = false;
	while (!end && out_offset < len) {
		out_len = len - out_offset;
		ret = ctx->algorithm->process(ctx->ud, (zip_uint8_t*) data + out_offset, &out_len);

		if (ret != ZIP_COMPRESSION_ERROR) {
			out_offset += out_len;
		}

		switch (ret) {
		case ZIP_COMPRESSION_END:
			ctx->end_of_stream = true;

			if (!ctx->end_of_input) {
				/* TODO: garbage after stream, or compression ended before all data read */
			}

			if (ctx->first_read < 0) {
				/* we got end of processed stream before reading any input data */
				zip_error_set(&ctx->error, ZIP_ER_INTERNAL, 0);
				end = true;
				break;
			}
			if (ctx->can_store && (zip_uint64_t) ctx->first_read <= out_offset) {
				ctx->is_stored = true;
				ctx->size = (zip_uint64_t) ctx->first_read;
				memcpy(data, ctx->buffer, ctx->size);
				return (zip_int64_t) ctx->size;
			}
			end = true;
			break;

		case ZIP_COMPRESSION_OK:
			break;

		case ZIP_COMPRESSION_NEED_DATA:
			if (ctx->end_of_input) {
				/* TODO: error: stream not ended, but no more input */
				end = true;
				break;
			}

			if ((n = zip_source_read(src, ctx->buffer, sizeof(ctx->buffer))) < 0) {
				_zip_error_set_from_source(&ctx->error, src);
				end = true;
				break;
			} else if (n == 0) {
				ctx->end_of_input = true;
				ctx->algorithm->end_of_input(ctx->ud);
				if (ctx->first_read < 0) {
					ctx->first_read = 0;
				}
			} else {
				if (ctx->first_read >= 0) {
					/* we overwrote a previously filled ctx->buffer */
					ctx->can_store = false;
				} else {
					ctx->first_read = n;
				}

				ctx->algorithm->input(ctx->ud, ctx->buffer, (zip_uint64_t) n);
			}
			break;

		case ZIP_COMPRESSION_ERROR:
			/* error set by algorithm */
			if (zip_error_code_zip(&ctx->error) == ZIP_ER_OK) {
				zip_error_set(&ctx->error, ZIP_ER_INTERNAL, 0);
			}
			end = true;
			break;
		}
	}

	if (out_offset > 0) {
		ctx->can_store = false;
		ctx->size += out_offset;
		return (zip_int64_t) out_offset;
	}

	return (zip_error_code_zip(&ctx->error) == ZIP_ER_OK) ? 0 : -1;
}

static zip_int64_t
compress_callback(zip_source_t *src, void *ud, void *data, zip_uint64_t len, zip_source_cmd_t cmd) {
	struct context *ctx;

	ctx = (struct context*) ud;

	switch (cmd) {
	case ZIP_SOURCE_OPEN:
		ctx->size = 0;
		ctx->end_of_input = false;
		ctx->end_of_stream = false;
		ctx->is_stored = false;
		ctx->first_read = -1;

		if (!ctx->algorithm->start(ctx->ud)) {
			return -1;
		}

		return 0;

	case ZIP_SOURCE_READ:
		return compress_read(src, ctx, data, len);

	case ZIP_SOURCE_CLOSE:
		if (!ctx->algorithm->end(ctx->ud)) {
			return -1;
		}
		return 0;

	case ZIP_SOURCE_STAT: {
		zip_stat_t *st;

		st = (zip_stat_t*) data;

		if (ctx->compress) {
			if (ctx->end_of_stream) {
				st->comp_method = ctx->is_stored ? ZIP_CM_STORE : ZIP_CM_ACTUAL(ctx->method);
				st->comp_size = ctx->size;
				st->valid |= ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD;
			} else {
				st->valid &= ~(ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD);
			}
		} else {
			st->comp_method = ZIP_CM_STORE;
			st->valid |= ZIP_STAT_COMP_METHOD;
			if (ctx->end_of_stream) {
				st->size = ctx->size;
				st->valid |= ZIP_STAT_SIZE;
			} else {
				st->valid &= ~ZIP_STAT_SIZE;
			}
		}
	}
		return 0;

	case ZIP_SOURCE_GET_COMPRESSION_FLAGS:
		return ctx->is_stored ? 0 : ctx->algorithm->compression_flags(ctx->ud);

	case ZIP_SOURCE_ERROR:
		return zip_error_to_data(&ctx->error, data, len);

	case ZIP_SOURCE_FREE:
		context_free(ctx);
		return 0;

	case ZIP_SOURCE_SUPPORTS:
		return ZIP_SOURCE_SUPPORTS_READABLE | zip_source_make_command_bitmap(ZIP_SOURCE_GET_COMPRESSION_FLAGS, -1);

	default:
		zip_error_set(&ctx->error, ZIP_ER_INTERNAL, 0);
		return -1;
	}
}

struct crc_context {
    int validate;     /* whether to check CRC on EOF and return error on mismatch */
    int crc_complete; /* whether CRC was computed for complete file */
    zip_error_t error;
    zip_uint64_t size;
    zip_uint64_t position;     /* current reading position */
    zip_uint64_t crc_position; /* how far we've computed the CRC */
    zip_uint32_t crc;
};

static zip_int64_t crc_read(zip_source_t *, void *, void *, zip_uint64_t, zip_source_cmd_t);

static zip_source_t *
zip_source_crc(zip_t *za, zip_source_t *src, int validate) {
	struct crc_context *ctx;

	if (src == NULL) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return NULL;
	}

	if ((ctx = (struct crc_context*) malloc(sizeof(*ctx))) == NULL) {
		zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
		return NULL;
	}

	zip_error_init(&ctx->error);
	ctx->validate = validate;
	ctx->crc_complete = 0;
	ctx->crc_position = 0;
	ctx->crc = (zip_uint32_t) crc32(0, NULL, 0);
	ctx->size = 0;

	return zip_source_layered(za, src, crc_read, ctx);
}


static zip_int64_t
crc_read(zip_source_t *src, void *_ctx, void *data, zip_uint64_t len, zip_source_cmd_t cmd) {
	struct crc_context *ctx;
	zip_int64_t n;

	ctx = (struct crc_context*) _ctx;

	switch (cmd) {
	case ZIP_SOURCE_OPEN:
		ctx->position = 0;
		return 0;

	case ZIP_SOURCE_READ:
		if ((n = zip_source_read(src, data, len)) < 0) {
			_zip_error_set_from_source(&ctx->error, src);
			return -1;
		}

		if (n == 0) {
			if (ctx->crc_position == ctx->position) {
				ctx->crc_complete = 1;
				ctx->size = ctx->position;

				if (ctx->validate) {
					struct zip_stat st;

					if (zip_source_stat(src, &st) < 0) {
						_zip_error_set_from_source(&ctx->error, src);
						return -1;
					}

					if ((st.valid & ZIP_STAT_CRC) && st.crc != ctx->crc) {
						zip_error_set(&ctx->error, ZIP_ER_CRC, 0);
						return -1;
					}
					if ((st.valid & ZIP_STAT_SIZE) && st.size != ctx->size) {
						zip_error_set(&ctx->error, ZIP_ER_INCONS, 0);
						return -1;
					}
				}
			}
		} else if (!ctx->crc_complete && ctx->position <= ctx->crc_position) {
			zip_uint64_t i, nn;

			for (i = ctx->crc_position - ctx->position; i < (zip_uint64_t) n; i += nn) {
				nn = ZIP_MIN(UINT_MAX, (zip_uint64_t )n - i);

				ctx->crc = (zip_uint32_t) crc32(ctx->crc, (const Bytef*) data + i, (uInt) nn);
				ctx->crc_position += nn;
			}
		}
		ctx->position += (zip_uint64_t) n;
		return n;

	case ZIP_SOURCE_CLOSE:
		return 0;

	case ZIP_SOURCE_STAT: {
		zip_stat_t *st;

		st = (zip_stat_t*) data;

		if (ctx->crc_complete) {
			/* TODO: Set comp_size, comp_method, encryption_method?
			 After all, this only works for uncompressed data. */
			st->size = ctx->size;
			st->crc = ctx->crc;
			st->comp_size = ctx->size;
			st->comp_method = ZIP_CM_STORE;
			st->encryption_method = ZIP_EM_NONE;
			st->valid |= ZIP_STAT_SIZE | ZIP_STAT_CRC | ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD
					| ZIP_STAT_ENCRYPTION_METHOD;
		}
		return 0;
	}

	case ZIP_SOURCE_ERROR:
		return zip_error_to_data(&ctx->error, data, len);

	case ZIP_SOURCE_FREE:
		free(ctx);
		return 0;

	case ZIP_SOURCE_SUPPORTS: {
		zip_int64_t mask = zip_source_supports(src);

		if (mask < 0) {
			_zip_error_set_from_source(&ctx->error, src);
			return -1;
		}

		return mask
				& ~zip_source_make_command_bitmap(ZIP_SOURCE_BEGIN_WRITE, ZIP_SOURCE_COMMIT_WRITE,
						ZIP_SOURCE_ROLLBACK_WRITE, ZIP_SOURCE_SEEK_WRITE, ZIP_SOURCE_TELL_WRITE, ZIP_SOURCE_REMOVE,
						ZIP_SOURCE_GET_COMPRESSION_FLAGS, -1);
	}

	case ZIP_SOURCE_SEEK: {
		zip_int64_t new_position;
		zip_source_args_seek_t *args = ZIP_SOURCE_GET_ARGS(zip_source_args_seek_t, data, len, &ctx->error);

		if (args == NULL) {
			return -1;
		}
		if (zip_source_seek(src, args->offset, args->whence) < 0 || (new_position = zip_source_tell(src)) < 0) {
			_zip_error_set_from_source(&ctx->error, src);
			return -1;
		}

		ctx->position = (zip_uint64_t) new_position;

		return 0;
	}

	case ZIP_SOURCE_TELL:
		return (zip_int64_t) ctx->position;

	default:
		zip_error_set(&ctx->error, ZIP_ER_OPNOTSUPP, 0);
		return -1;
	}
}
