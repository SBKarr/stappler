/*
  zip_error_clear.c -- clear zip error
  Copyright (C) 1999-2019 Dieter Baron and Thomas Klausner

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

static const char * const _zip_err_str[] = {
    "No error",
    "Multi-disk zip archives not supported",
    "Renaming temporary file failed",
    "Closing zip archive failed",
    "Seek error",
    "Read error",
    "Write error",
    "CRC error",
    "Containing zip archive was closed",
    "No such file",
    "File already exists",
    "Can't open file",
    "Failure to create temporary file",
    "Zlib error",
    "Malloc failure",
    "Entry has been changed",
    "Compression method not supported",
    "Premature end of file",
    "Invalid argument",
    "Not a zip archive",
    "Internal error",
    "Zip archive inconsistent",
    "Can't remove file",
    "Entry has been deleted",
    "Encryption method not supported",
    "Read-only archive",
    "No password provided",
    "Wrong password provided",
    "Operation not supported",
    "Resource still in use",
    "Tell error",
    "Compressed data invalid",
    "Operation cancelled",
};

static const int _zip_nerr_str = sizeof(_zip_err_str)/sizeof(_zip_err_str[0]);

#define N ZIP_ET_NONE
#define S ZIP_ET_SYS
#define Z ZIP_ET_ZLIB

static const int _zip_err_type[] = {
    N,
    N,
    S,
    S,
    S,
    S,
    S,
    N,
    N,
    N,
    N,
    S,
    S,
    Z,
    N,
    N,
    N,
    N,
    N,
    N,
    N,
    N,
    S,
    N,
    N,
    N,
    N,
    N,
    N,
    N,
    S,
    N,
    N,
};


ZIP_EXTERN zip_error_t * zip_get_error(zip_t *za) {
    if (za == NULL) {
    	return NULL;
    }
	return &za->error;
}

ZIP_EXTERN void
zip_error_clear(zip_t *za) {
    if (za == NULL)
	return;

    _zip_error_clear(&za->error);
}

ZIP_EXTERN const char*
zip_error_strerror(zip_error_t *err) {
	const char *zs, *ss;
	char buf[128], *s;

	zip_error_fini(err);

	if (err->zip_err < 0 || err->zip_err >= _zip_nerr_str) {
		sprintf(buf, "Unknown error %d", err->zip_err);
		zs = NULL;
		ss = buf;
	} else {
		zs = _zip_err_str[err->zip_err];

		switch (_zip_err_type[err->zip_err]) {
		case ZIP_ET_SYS:
			ss = strerror(err->sys_err);
			break;

		case ZIP_ET_ZLIB:
			ss = zError(err->sys_err);
			break;

		default:
			ss = NULL;
		}
	}

	if (ss == NULL)
		return zs;
	else {
		if ((s = (char*) malloc(strlen(ss) + (zs ? strlen(zs) + 2 : 0) + 1)) == NULL)
			return _zip_err_str[ZIP_ER_MEMORY];

		sprintf(s, "%s%s%s", (zs ? zs : ""), (zs ? ": " : ""), ss);
		err->str = s;

		return s;
	}
}

ZIP_EXTERN int
zip_error_code_system(const zip_error_t *error) {
    return error->sys_err;
}

ZIP_EXTERN int
zip_error_code_zip(const zip_error_t *error) {
    return error->zip_err;
}

ZIP_EXTERN void
zip_error_fini(zip_error_t *err) {
    free(err->str);
    err->str = NULL;
}

ZIP_EXTERN void
zip_error_init(zip_error_t *err) {
    err->zip_err = ZIP_ER_OK;
    err->sys_err = 0;
    err->str = NULL;
}

ZIP_EXTERN void
zip_error_init_with_code(zip_error_t *error, int ze) {
    zip_error_init(error);
    error->zip_err = ze;
    switch (zip_error_system_type(error)) {
    case ZIP_ET_SYS:
	error->sys_err = errno;
	break;

    default:
	error->sys_err = 0;
	break;
    }
}

ZIP_EXTERN int
zip_error_system_type(const zip_error_t *error) {
    if (error->zip_err < 0 || error->zip_err >= _zip_nerr_str)
	return ZIP_ET_NONE;

    return _zip_err_type[error->zip_err];
}

static void
_zip_error_clear(zip_error_t *err) {
    if (err == NULL)
	return;

    err->zip_err = ZIP_ER_OK;
    err->sys_err = 0;
}

static void
_zip_error_copy(zip_error_t *dst, const zip_error_t *src) {
    if (dst == NULL) {
	return;
    }

    dst->zip_err = src->zip_err;
    dst->sys_err = src->sys_err;
}

void
zip_error_set(zip_error_t *err, int ze, int se) {
    if (err) {
	err->zip_err = ze;
	err->sys_err = se;
    }
}

static void
_zip_error_set_from_source(zip_error_t *err, zip_source_t *src) {
    _zip_error_copy(err, zip_source_error(src));
}

zip_int64_t
zip_error_to_data(const zip_error_t *error, void *data, zip_uint64_t length) {
    int *e = (int *)data;

    if (length < sizeof(int) * 2) {
	return -1;
    }

    e[0] = zip_error_code_zip(error);
    e[1] = zip_error_code_system(error);
    return sizeof(int) * 2;
}

#undef N
#undef S
#undef Z
