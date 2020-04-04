/*
  zip_source_accept_empty.c -- if empty source is a valid archive
  Copyright (C) 2019 Dieter Baron and Thomas Klausner

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

static zip_int64_t
zip_source_supports(zip_source_t *src) {
    return src->supports;
}

static bool
zip_source_accept_empty(zip_source_t *src) {
	int ret;

	if ((zip_source_supports(src) & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_ACCEPT_EMPTY)) == 0) {
		if (ZIP_SOURCE_IS_LAYERED(src)) {
			return zip_source_accept_empty(src->src);
		}
		return true;
	}

	ret = (int) _zip_source_call(src, NULL, 0, ZIP_SOURCE_ACCEPT_EMPTY);

	return ret != 0;
}

ZIP_EXTERN int
zip_source_begin_write_cloning(zip_source_t *src, zip_uint64_t offset) {
	if (ZIP_SOURCE_IS_OPEN_WRITING(src)) {
		zip_error_set(&src->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (_zip_source_call(src, NULL, offset, ZIP_SOURCE_BEGIN_WRITE_CLONING) < 0) {
		return -1;
	}

	src->write_state = ZIP_SOURCE_WRITE_OPEN;

	return 0;
}

ZIP_EXTERN int
zip_source_begin_write(zip_source_t *src) {
	if (ZIP_SOURCE_IS_OPEN_WRITING(src)) {
		zip_error_set(&src->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (_zip_source_call(src, NULL, 0, ZIP_SOURCE_BEGIN_WRITE) < 0) {
		return -1;
	}

	src->write_state = ZIP_SOURCE_WRITE_OPEN;

	return 0;
}


static zip_int64_t
_zip_source_call(zip_source_t *src, void *data, zip_uint64_t length, zip_source_cmd_t command) {
	zip_int64_t ret;

	if ((src->supports & ZIP_SOURCE_MAKE_COMMAND_BITMASK(command)) == 0) {
		zip_error_set(&src->error, ZIP_ER_OPNOTSUPP, 0);
		return -1;
	}

	if (src->src == NULL) {
		ret = src->cb.f(src->ud, data, length, command);
	} else {
		ret = src->cb.l(src->src, src->ud, data, length, command);
	}

	if (ret < 0) {
		if (command != ZIP_SOURCE_ERROR && command != ZIP_SOURCE_SUPPORTS) {
			int e[2];

			if (_zip_source_call(src, e, sizeof(e), ZIP_SOURCE_ERROR) < 0) {
				zip_error_set(&src->error, ZIP_ER_INTERNAL, 0);
			} else {
				zip_error_set(&src->error, e[0], e[1]);
			}
		}
	}

	return ret;
}

int
zip_source_close(zip_source_t *src) {
	if (!ZIP_SOURCE_IS_OPEN_READING(src)) {
		zip_error_set(&src->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	src->open_count--;
	if (src->open_count == 0) {
		_zip_source_call(src, NULL, 0, ZIP_SOURCE_CLOSE);

		if (ZIP_SOURCE_IS_LAYERED(src)) {
			if (zip_source_close(src->src) < 0) {
				zip_error_set(&src->error, ZIP_ER_INTERNAL, 0);
			}
		}
	}

	return 0;
}

ZIP_EXTERN int
zip_source_commit_write(zip_source_t *src) {
	if (!ZIP_SOURCE_IS_OPEN_WRITING(src)) {
		zip_error_set(&src->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (src->open_count > 1) {
		zip_error_set(&src->error, ZIP_ER_INUSE, 0);
		return -1;
	} else if (ZIP_SOURCE_IS_OPEN_READING(src)) {
		if (zip_source_close(src) < 0) {
			return -1;
		}
	}

	if (_zip_source_call(src, NULL, 0, ZIP_SOURCE_COMMIT_WRITE) < 0) {
		src->write_state = ZIP_SOURCE_WRITE_FAILED;
		return -1;
	}

	src->write_state = ZIP_SOURCE_WRITE_CLOSED;

	return 0;
}

zip_error_t *
zip_source_error(zip_source_t *src) {
    return &src->error;
}

ZIP_EXTERN void
zip_source_free(zip_source_t *src) {
	if (src == NULL)
		return;

	if (src->refcount > 0) {
		src->refcount--;
	}
	if (src->refcount > 0) {
		return;
	}

	if (ZIP_SOURCE_IS_OPEN_READING(src)) {
		src->open_count = 1; /* force close */
		zip_source_close(src);
	}
	if (ZIP_SOURCE_IS_OPEN_WRITING(src)) {
		zip_source_rollback_write(src);
	}

	if (src->source_archive && !src->source_closed) {
		_zip_deregister_source(src->source_archive, src);
	}

	(void) _zip_source_call(src, NULL, 0, ZIP_SOURCE_FREE);

	if (src->src) {
		zip_source_free(src->src);
	}

	free(src);
}

ZIP_EXTERN zip_source_t *
zip_source_function(zip_t *za, zip_source_callback zcb, void *ud) {
	if (za == NULL) {
		return NULL;
	}

	return zip_source_function_create(zcb, ud, &za->error);
}


ZIP_EXTERN zip_source_t *
zip_source_function_create(zip_source_callback zcb, void *ud, zip_error_t *error) {
	zip_source_t *zs;

	if ((zs = _zip_source_new(error)) == NULL)
		return NULL;

	zs->cb.f = zcb;
	zs->ud = ud;

	zs->supports = zcb(ud, NULL, 0, ZIP_SOURCE_SUPPORTS);
	if (zs->supports < 0) {
		zs->supports = ZIP_SOURCE_SUPPORTS_READABLE;
	}

	return zs;
}

ZIP_EXTERN void
zip_source_keep(zip_source_t *src) {
    src->refcount++;
}

static zip_source_t *
_zip_source_new(zip_error_t *error) {
	zip_source_t *src;

	if ((src = (zip_source_t*) malloc(sizeof(*src))) == NULL) {
		zip_error_set(error, ZIP_ER_MEMORY, 0);
		return NULL;
	}

	src->src = NULL;
	src->cb.f = NULL;
	src->ud = NULL;
	src->open_count = 0;
	src->write_state = ZIP_SOURCE_WRITE_CLOSED;
	src->source_closed = false;
	src->source_archive = NULL;
	src->refcount = 1;
	zip_error_init(&src->error);
	src->eof = false;
	src->had_read_error = false;

	return src;
}

#define ZIP_COMPRESSION_BITFLAG_MAX 3

static zip_int8_t
zip_source_get_compression_flags(zip_source_t *src) {
	while (src) {
		if ((src->supports & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_GET_COMPRESSION_FLAGS))) {
			zip_int64_t ret = _zip_source_call(src, NULL, 0, ZIP_SOURCE_GET_COMPRESSION_FLAGS);
			if (ret < 0) {
				return -1;
			}
			if (ret > ZIP_COMPRESSION_BITFLAG_MAX) {
				zip_error_set(&src->error, ZIP_ER_INTERNAL, 0);
				return -1;
			}
			return (zip_int8_t) ret;
		}
		src = src->src;
	}

	return 0;
}

ZIP_EXTERN int
zip_source_is_deleted(zip_source_t *src) {
    return src->write_state == ZIP_SOURCE_WRITE_REMOVED;
}

ZIP_EXTERN int
zip_source_open(zip_source_t *src) {
	if (src->source_closed) {
		return -1;
	}
	if (src->write_state == ZIP_SOURCE_WRITE_REMOVED) {
		zip_error_set(&src->error, ZIP_ER_DELETED, 0);
		return -1;
	}

	if (ZIP_SOURCE_IS_OPEN_READING(src)) {
		if ((zip_source_supports(src) & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_SEEK)) == 0) {
			zip_error_set(&src->error, ZIP_ER_INUSE, 0);
			return -1;
		}
	} else {
		if (ZIP_SOURCE_IS_LAYERED(src)) {
			if (zip_source_open(src->src) < 0) {
				_zip_error_set_from_source(&src->error, src->src);
				return -1;
			}
		}

		if (_zip_source_call(src, NULL, 0, ZIP_SOURCE_OPEN) < 0) {
			if (ZIP_SOURCE_IS_LAYERED(src)) {
				zip_source_close(src->src);
			}
			return -1;
		}
	}

	src->eof = false;
	src->had_read_error = false;
	_zip_error_clear(&src->error);
	src->open_count++;

	return 0;
}

zip_int64_t
zip_source_read(zip_source_t *src, void *data, zip_uint64_t len) {
	zip_uint64_t bytes_read;
	zip_int64_t n;

	if (src->source_closed) {
		return -1;
	}
	if (!ZIP_SOURCE_IS_OPEN_READING(src) || len > ZIP_INT64_MAX || (len > 0 && data == NULL)) {
		zip_error_set(&src->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (src->had_read_error) {
		return -1;
	}

	if (_zip_source_eof(src)) {
		return 0;
	}

	if (len == 0) {
		return 0;
	}

	bytes_read = 0;
	while (bytes_read < len) {
		if ((n = _zip_source_call(src, (zip_uint8_t*) data + bytes_read, len - bytes_read, ZIP_SOURCE_READ)) < 0) {
			src->had_read_error = true;
			if (bytes_read == 0) {
				return -1;
			} else {
				return (zip_int64_t) bytes_read;
			}
		}

		if (n == 0) {
			src->eof = 1;
			break;
		}

		bytes_read += (zip_uint64_t) n;
	}

	return (zip_int64_t) bytes_read;
}

static bool
_zip_source_eof(zip_source_t *src) {
    return src->eof;
}

ZIP_EXTERN void
zip_source_rollback_write(zip_source_t *src) {
	if (src->write_state != ZIP_SOURCE_WRITE_OPEN && src->write_state != ZIP_SOURCE_WRITE_FAILED) {
		return;
	}

	_zip_source_call(src, NULL, 0, ZIP_SOURCE_ROLLBACK_WRITE);
	src->write_state = ZIP_SOURCE_WRITE_CLOSED;
}

ZIP_EXTERN int
zip_source_seek_write(zip_source_t *src, zip_int64_t offset, int whence) {
    zip_source_args_seek_t args;

    if (!ZIP_SOURCE_IS_OPEN_WRITING(src) || (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)) {
	zip_error_set(&src->error, ZIP_ER_INVAL, 0);
	return -1;
    }

    args.offset = offset;
    args.whence = whence;

    return (_zip_source_call(src, &args, sizeof(args), ZIP_SOURCE_SEEK_WRITE) < 0 ? -1 : 0);
}

ZIP_EXTERN int
zip_source_seek(zip_source_t *src, zip_int64_t offset, int whence) {
	zip_source_args_seek_t args;

	if (src->source_closed) {
		return -1;
	}
	if (!ZIP_SOURCE_IS_OPEN_READING(src) || (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)) {
		zip_error_set(&src->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	args.offset = offset;
	args.whence = whence;

	if (_zip_source_call(src, &args, sizeof(args), ZIP_SOURCE_SEEK) < 0) {
		return -1;
	}

	src->eof = 0;
	return 0;
}

zip_int64_t
zip_source_seek_compute_offset(zip_uint64_t offset, zip_uint64_t length, void *data, zip_uint64_t data_length, zip_error_t *error) {
	zip_int64_t new_offset;
	zip_source_args_seek_t *args = ZIP_SOURCE_GET_ARGS(zip_source_args_seek_t, data, data_length, error);

	if (args == NULL) {
		return -1;
	}

	switch (args->whence) {
	case SEEK_CUR:
		new_offset = (zip_int64_t) offset + args->offset;
		break;

	case SEEK_END:
		new_offset = (zip_int64_t) length + args->offset;
		break;

	case SEEK_SET:
		new_offset = args->offset;
		break;

	default:
		zip_error_set(error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (new_offset < 0 || (zip_uint64_t) new_offset > length) {
		zip_error_set(error, ZIP_ER_INVAL, 0);
		return -1;
	}

	return new_offset;
}

ZIP_EXTERN int
zip_source_stat(zip_source_t *src, zip_stat_t *st) {
	if (src->source_closed) {
		return -1;
	}
	if (st == NULL) {
		zip_error_set(&src->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	zip_stat_init(st);

	if (ZIP_SOURCE_IS_LAYERED(src)) {
		if (zip_source_stat(src->src, st) < 0) {
			_zip_error_set_from_source(&src->error, src->src);
			return -1;
		}
	}

	if (_zip_source_call(src, st, sizeof(*st), ZIP_SOURCE_STAT) < 0) {
		return -1;
	}

	return 0;
}

ZIP_EXTERN zip_int64_t
zip_source_make_command_bitmap(zip_source_cmd_t cmd0, ...) {
	zip_int64_t bitmap;
	va_list ap;

	bitmap = ZIP_SOURCE_MAKE_COMMAND_BITMASK(cmd0);

	va_start(ap, cmd0);
	for (;;) {
		int cmd = va_arg(ap, int);
		if (cmd < 0) {
			break;
		}
		bitmap |= ZIP_SOURCE_MAKE_COMMAND_BITMASK(cmd);
	}
	va_end(ap);

	return bitmap;
}

ZIP_EXTERN zip_int64_t
zip_source_tell_write(zip_source_t *src) {
    if (!ZIP_SOURCE_IS_OPEN_WRITING(src)) {
	zip_error_set(&src->error, ZIP_ER_INVAL, 0);
	return -1;
    }

    return _zip_source_call(src, NULL, 0, ZIP_SOURCE_TELL_WRITE);
}

ZIP_EXTERN zip_int64_t
zip_source_tell(zip_source_t *src) {
	if (src->source_closed) {
		return -1;
	}
	if (!ZIP_SOURCE_IS_OPEN_READING(src)) {
		zip_error_set(&src->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	return _zip_source_call(src, NULL, 0, ZIP_SOURCE_TELL);
}

ZIP_EXTERN zip_int64_t
zip_source_write(zip_source_t *src, const void *data, zip_uint64_t length) {
    if (!ZIP_SOURCE_IS_OPEN_WRITING(src) || length > ZIP_INT64_MAX) {
	zip_error_set(&src->error, ZIP_ER_INVAL, 0);
	return -1;
    }

    return _zip_source_call(src, (void *)data, length, ZIP_SOURCE_WRITE);
}

ZIP_EXTERN zip_source_t *
zip_source_zip(zip_t *za, zip_t *srcza, zip_uint64_t srcidx, zip_flags_t flags, zip_uint64_t start, zip_int64_t len) {
    if (len < -1) {
	zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	return NULL;
    }

    if (len == -1)
	len = 0;

    if (start == 0 && len == 0)
	flags |= ZIP_FL_COMPRESSED;
    else
	flags &= ~ZIP_FL_COMPRESSED;

    return _zip_source_zip_new(za, srcza, srcidx, flags, start, (zip_uint64_t)len, NULL);
}

struct window {
    zip_uint64_t start; /* where in file we start reading */
    zip_uint64_t end;   /* where in file we stop reading */

    /* if not NULL, read file data for this file */
    zip_t *source_archive;
    zip_uint64_t source_index;

    zip_uint64_t offset; /* offset in src for next read */

    zip_stat_t stat;
    zip_int8_t compression_flags;
    zip_error_t error;
    zip_int64_t supports;
    bool needs_seek;
};

static zip_int64_t window_read(zip_source_t *, void *, void *, zip_uint64_t, zip_source_cmd_t);

static zip_source_t *
zip_source_window(zip_t *za, zip_source_t *src, zip_uint64_t start, zip_uint64_t len) {
    return _zip_source_window_new(src, start, len, NULL, 0, NULL, 0, &za->error);
}

static zip_source_t *
_zip_source_window_new(zip_source_t *src, zip_uint64_t start, zip_uint64_t length, zip_stat_t *st, zip_int8_t compression_flags, zip_t *source_archive, zip_uint64_t source_index, zip_error_t *error) {
    struct window *ctx;

    if (src == NULL || start + length < start || (source_archive == NULL && source_index != 0)) {
	zip_error_set(error, ZIP_ER_INVAL, 0);
	return NULL;
    }

    if ((ctx = (struct window *)malloc(sizeof(*ctx))) == NULL) {
	zip_error_set(error, ZIP_ER_MEMORY, 0);
	return NULL;
    }

    ctx->start = start;
    ctx->end = start + length;
    zip_stat_init(&ctx->stat);
    ctx->compression_flags = compression_flags;
    ctx->source_archive = source_archive;
    ctx->source_index = source_index;
    zip_error_init(&ctx->error);
    ctx->supports = (zip_source_supports(src) & ZIP_SOURCE_SUPPORTS_SEEKABLE) | (zip_source_make_command_bitmap(ZIP_SOURCE_GET_COMPRESSION_FLAGS, ZIP_SOURCE_SUPPORTS, ZIP_SOURCE_TELL, -1));
    ctx->needs_seek = (ctx->supports & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_SEEK)) ? true : false;

    if (st) {
	if (_zip_stat_merge(&ctx->stat, st, error) < 0) {
	    free(ctx);
	    return NULL;
	}
    }

    return zip_source_layered_create(src, window_read, ctx, error);
}

static int
_zip_source_set_source_archive(zip_source_t *src, zip_t *za) {
    src->source_archive = za;
    return _zip_register_source(za, src);
}

static zip_int64_t
window_read(zip_source_t *src, void *_ctx, void *data, zip_uint64_t len, zip_source_cmd_t cmd) {
    struct window *ctx;
    zip_int64_t ret;
    zip_uint64_t n, i;

    ctx = (struct window *)_ctx;

    switch (cmd) {
    case ZIP_SOURCE_CLOSE:
	return 0;

    case ZIP_SOURCE_ERROR:
	return zip_error_to_data(&ctx->error, data, len);

    case ZIP_SOURCE_FREE:
	free(ctx);
	return 0;

    case ZIP_SOURCE_OPEN:
	if (ctx->source_archive) {
	    zip_uint64_t offset;

	    if ((offset = _zip_file_get_offset(ctx->source_archive, ctx->source_index, &ctx->error)) == 0) {
		return -1;
	    }
	    if (ctx->end + offset < ctx->end) {
		/* zip archive data claims end of data past zip64 limits */
		zip_error_set(&ctx->error, ZIP_ER_INCONS, 0);
		return -1;
	    }
	    ctx->start += offset;
	    ctx->end += offset;
	    ctx->source_archive = NULL;
	}

	if (!ctx->needs_seek) {
	    DEFINE_BYTE_ARRAY(b, BUFSIZE);

	    if (!byte_array_init(b, BUFSIZE)) {
		zip_error_set(&ctx->error, ZIP_ER_MEMORY, 0);
		return -1;
	    }

	    for (n = 0; n < ctx->start; n += (zip_uint64_t)ret) {
		i = (ctx->start - n > BUFSIZE ? BUFSIZE : ctx->start - n);
		if ((ret = zip_source_read(src, b, i)) < 0) {
		    _zip_error_set_from_source(&ctx->error, src);
		    byte_array_fini(b);
		    return -1;
		}
		if (ret == 0) {
		    zip_error_set(&ctx->error, ZIP_ER_EOF, 0);
		    byte_array_fini(b);
		    return -1;
		}
	    }

	    byte_array_fini(b);
	}

	ctx->offset = ctx->start;
	return 0;

    case ZIP_SOURCE_READ:
	if (len > ctx->end - ctx->offset)
	    len = ctx->end - ctx->offset;

	if (len == 0)
	    return 0;

	if (ctx->needs_seek) {
	    if (zip_source_seek(src, (zip_int64_t)ctx->offset, SEEK_SET) < 0) {
		_zip_error_set_from_source(&ctx->error, src);
		return -1;
	    }
	}

	if ((ret = zip_source_read(src, data, len)) < 0) {
	    zip_error_set(&ctx->error, ZIP_ER_EOF, 0);
	    return -1;
	}

	ctx->offset += (zip_uint64_t)ret;

	if (ret == 0) {
	    if (ctx->offset < ctx->end) {
		zip_error_set(&ctx->error, ZIP_ER_EOF, 0);
		return -1;
	    }
	}
	return ret;

    case ZIP_SOURCE_SEEK: {
	zip_int64_t new_offset = zip_source_seek_compute_offset(ctx->offset - ctx->start, ctx->end - ctx->start, data, len, &ctx->error);

	if (new_offset < 0) {
	    return -1;
	}

	ctx->offset = (zip_uint64_t)new_offset + ctx->start;
	return 0;
    }

    case ZIP_SOURCE_STAT: {
	zip_stat_t *st;

	st = (zip_stat_t *)data;

	if (_zip_stat_merge(st, &ctx->stat, &ctx->error) < 0) {
	    return -1;
	}
	return 0;
    }

    case ZIP_SOURCE_GET_COMPRESSION_FLAGS:
	return ctx->compression_flags;

    case ZIP_SOURCE_SUPPORTS:
	return ctx->supports;

    case ZIP_SOURCE_TELL:
	return (zip_int64_t)(ctx->offset - ctx->start);

    default:
	zip_error_set(&ctx->error, ZIP_ER_OPNOTSUPP, 0);
	return -1;
    }
}


static void
_zip_deregister_source(zip_t *za, zip_source_t *src) {
    unsigned int i;

    for (i = 0; i < za->nopen_source; i++) {
	if (za->open_source[i] == src) {
	    za->open_source[i] = za->open_source[za->nopen_source - 1];
	    za->nopen_source--;
	    break;
	}
    }
}


static int
_zip_register_source(zip_t *za, zip_source_t *src) {
    zip_source_t **open_source;

    if (za->nopen_source + 1 >= za->nopen_source_alloc) {
	unsigned int n;
	n = za->nopen_source_alloc + 10;
	open_source = (zip_source_t **)realloc(za->open_source, n * sizeof(zip_source_t *));
	if (open_source == NULL) {
	    zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
	    return -1;
	}
	za->nopen_source_alloc = n;
	za->open_source = open_source;
    }

    za->open_source[za->nopen_source++] = src;

    return 0;
}

static zip_source_t *
_zip_source_zip_new(zip_t *za, zip_t *srcza, zip_uint64_t srcidx, zip_flags_t flags, zip_uint64_t start, zip_uint64_t len, const char *password) {
	zip_source_t *src, *s2;
	struct zip_stat st;
	bool partial_data, needs_crc, needs_decrypt, needs_decompress;

	if (za == NULL)
		return NULL;

	if (srcza == NULL || srcidx >= srcza->nentry) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return NULL;
	}

	if ((flags & ZIP_FL_UNCHANGED) == 0
			&& (ZIP_ENTRY_DATA_CHANGED(srcza->entry + srcidx) || srcza->entry[srcidx].deleted)) {
		zip_error_set(&za->error, ZIP_ER_CHANGED, 0);
		return NULL;
	}

	if (zip_stat_index(srcza, srcidx, flags | ZIP_FL_UNCHANGED, &st) < 0) {
		zip_error_set(&za->error, ZIP_ER_INTERNAL, 0);
		return NULL;
	}

	if (flags & ZIP_FL_ENCRYPTED)
		flags |= ZIP_FL_COMPRESSED;

	if ((start > 0 || len > 0) && (flags & ZIP_FL_COMPRESSED)) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return NULL;
	}

	/* overflow or past end of file */
	if ((start > 0 || len > 0) && (start + len < start || start + len > st.size)) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return NULL;
	}

	if (len == 0) {
		len = st.size - start;
	}

	partial_data = len < st.size;
	needs_decrypt = ((flags & ZIP_FL_ENCRYPTED) == 0) && (st.encryption_method != ZIP_EM_NONE);
	needs_decompress = ((flags & ZIP_FL_COMPRESSED) == 0) && (st.comp_method != ZIP_CM_STORE);
	/* when reading the whole file, check for CRC errors */
	needs_crc = ((flags & ZIP_FL_COMPRESSED) == 0 || st.comp_method == ZIP_CM_STORE) && !partial_data;

	if (needs_decrypt) {
		if (password == NULL) {
			password = za->default_password;
		}
		if (password == NULL) {
			zip_error_set(&za->error, ZIP_ER_NOPASSWD, 0);
			return NULL;
		}
	}

	if (st.comp_size == 0) {
		return zip_source_buffer(za, NULL, 0, 0);
	}

	if (partial_data && !needs_decrypt && !needs_decompress) {
		struct zip_stat st2;

		st2.size = len;
		st2.comp_size = len;
		st2.comp_method = ZIP_CM_STORE;
		st2.mtime = st.mtime;
		st2.valid = ZIP_STAT_SIZE | ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD | ZIP_STAT_MTIME;

		if ((src = _zip_source_window_new(srcza->src, start, len, &st2, 0, srcza, srcidx, &za->error)) == NULL) {
			return NULL;
		}
	} else {
		zip_dirent_t *de;

		if ((de = _zip_get_dirent(srcza, srcidx, flags, &za->error)) == NULL) {
			return NULL;
		}
		if ((src = _zip_source_window_new(srcza->src, 0, st.comp_size, &st, (de->bitflags >> 1) & 3, srcza, srcidx,
				&za->error)) == NULL) {
			return NULL;
		}
	}

	if (_zip_source_set_source_archive(src, srcza) < 0) {
		zip_source_free(src);
		return NULL;
	}

	/* creating a layered source calls zip_keep() on the lower layer, so we free it */

	if (needs_decrypt) {
		/*zip_encryption_implementation enc_impl;

		if ((enc_impl = _zip_get_encryption_implementation(st.encryption_method, ZIP_CODEC_DECODE)) == NULL) {
			zip_error_set(&za->error, ZIP_ER_ENCRNOTSUPP, 0);
			return NULL;
		}

		s2 = enc_impl(za, src, st.encryption_method, 0, password);
		zip_source_free(src);
		if (s2 == NULL) {
			return NULL;
		}
		src = s2;*/
		return NULL;
	}
	if (needs_decompress) {
		s2 = zip_source_decompress(za, src, st.comp_method);
		zip_source_free(src);
		if (s2 == NULL) {
			return NULL;
		}
		src = s2;
	}
	if (needs_crc) {
		s2 = zip_source_crc(za, src, 1);
		zip_source_free(src);
		if (s2 == NULL) {
			return NULL;
		}
		src = s2;
	}

	if (partial_data && (needs_decrypt || needs_decompress)) {
		s2 = zip_source_window(za, src, start, len);
		zip_source_free(src);
		if (s2 == NULL) {
			return NULL;
		}
		src = s2;
	}

	return src;
}
