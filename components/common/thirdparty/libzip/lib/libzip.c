/*
  zipint.h -- internal declarations.
  Copyright (C) 1999-2020 Dieter Baron and Thomas Klausner

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

static  zip_int64_t
_zip_add_entry(zip_t *za) {
    zip_uint64_t idx;

    if (za->nentry + 1 >= za->nentry_alloc) {
		zip_entry_t *rentries;
		zip_uint64_t nalloc = za->nentry_alloc;
		zip_uint64_t additional_entries = 2 * nalloc;
		zip_uint64_t realloc_size;

		if (additional_entries < 16) {
			additional_entries = 16;
		}
		else if (additional_entries > 1024) {
			additional_entries = 1024;
		}
		/* neither + nor * overflows can happen: nentry_alloc * sizeof(struct zip_entry) < UINT64_MAX */
		nalloc += additional_entries;
		realloc_size = sizeof(struct zip_entry) * (size_t)nalloc;

		if (sizeof(struct zip_entry) * (size_t)za->nentry_alloc > realloc_size) {
			zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
			return -1;
		}
		rentries = (zip_entry_t *)realloc(za->entry, sizeof(struct zip_entry) * (size_t)nalloc);
		if (!rentries) {
			zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
			return -1;
		}
		za->entry = rentries;
		za->nentry_alloc = nalloc;
    }

    idx = za->nentry++;

    _zip_entry_init(za->entry + idx);

    return (zip_int64_t)idx;
}

static void
_zip_entry_finalize(zip_entry_t *e) {
    _zip_unchange_data(e);
    _zip_dirent_free(e->orig);
    _zip_dirent_free(e->changes);
}


static void
_zip_entry_init(zip_entry_t *e) {
    e->orig = NULL;
    e->changes = NULL;
    e->source = NULL;
    e->deleted = 0;
}

static zip_uint32_t
_zip_string_crc32(const zip_string_t *s) {
	zip_uint32_t crc;

	crc = (zip_uint32_t) crc32(0L, Z_NULL, 0);

	if (s != NULL)
		crc = (zip_uint32_t) crc32(crc, s->raw, s->length);

	return crc;
}

static int
_zip_string_equal(const zip_string_t *a, const zip_string_t *b) {
	if (a == NULL || b == NULL)
		return a == b;

	if (a->length != b->length)
		return 0;

	/* TODO: encoding */

	return (memcmp(a->raw, b->raw, a->length) == 0);
}

static void
_zip_string_free(zip_string_t *s) {
	if (s == NULL)
		return;

	free(s->raw);
	free(s->converted);
	free(s);
}


static const zip_uint8_t *
_zip_string_get(zip_string_t *string, zip_uint32_t *lenp, zip_flags_t flags, zip_error_t *error) {
	static const zip_uint8_t empty[1] = "";

	if (string == NULL) {
		if (lenp)
			*lenp = 0;
		return empty;
	}

	if ((flags & ZIP_FL_ENC_RAW) == 0) {
		/* start guessing */
		if (string->encoding == ZIP_ENCODING_UNKNOWN)
			_zip_guess_encoding(string, ZIP_ENCODING_UNKNOWN);

		if (((flags & ZIP_FL_ENC_STRICT) && string->encoding != ZIP_ENCODING_ASCII
				&& string->encoding != ZIP_ENCODING_UTF8_KNOWN) || (string->encoding == ZIP_ENCODING_CP437)) {
			if (string->converted == NULL) {
				if ((string->converted = _zip_cp437_to_utf8(string->raw, string->length, &string->converted_length,
						error)) == NULL)
					return NULL;
			}
			if (lenp)
				*lenp = string->converted_length;
			return string->converted;
		}
	}

	if (lenp)
		*lenp = string->length;
	return string->raw;
}

static zip_uint16_t
_zip_string_length(const zip_string_t *s) {
	if (s == NULL)
		return 0;

	return s->length;
}

static zip_string_t *
_zip_string_new(const zip_uint8_t *raw, zip_uint16_t length, zip_flags_t flags, zip_error_t *error) {
	zip_string_t *s;
	zip_encoding_type_t expected_encoding;

	if (length == 0)
		return NULL;

	switch (flags & ZIP_FL_ENCODING_ALL) {
	case ZIP_FL_ENC_GUESS:
		expected_encoding = ZIP_ENCODING_UNKNOWN;
		break;
	case ZIP_FL_ENC_UTF_8:
		expected_encoding = ZIP_ENCODING_UTF8_KNOWN;
		break;
	case ZIP_FL_ENC_CP437:
		expected_encoding = ZIP_ENCODING_CP437;
		break;
	default:
		zip_error_set(error, ZIP_ER_INVAL, 0);
		return NULL;
	}

	if ((s = (zip_string_t*) malloc(sizeof(*s))) == NULL) {
		zip_error_set(error, ZIP_ER_MEMORY, 0);
		return NULL;
	}

	if ((s->raw = (zip_uint8_t*) malloc((size_t) (length + 1))) == NULL) {
		free(s);
		return NULL;
	}

	memcpy(s->raw, raw, length);
	s->raw[length] = '\0';
	s->length = length;
	s->encoding = ZIP_ENCODING_UNKNOWN;
	s->converted = NULL;
	s->converted_length = 0;

	if (expected_encoding != ZIP_ENCODING_UNKNOWN) {
		if (_zip_guess_encoding(s, expected_encoding) == ZIP_ENCODING_ERROR) {
			_zip_string_free(s);
			zip_error_set(error, ZIP_ER_INVAL, 0);
			return NULL;
		}
	}

	return s;
}

static int
_zip_string_write(zip_t *za, const zip_string_t *s) {
	if (s == NULL)
		return 0;

	return _zip_write(za, s->raw, s->length);
}

#include "zip_api.cc"
#include "zip_buffer.cc"
#include "zip_close.cc"
#include "zip_crypto_mbedtls.cc"
#include "zip_deflate.cc"
#include "zip_dirent.cc"
#include "zip_error.cc"
#include "zip_extra_field_api.cc"
#include "zip_file.cc"
#include "zip_hash.cc"
#include "zip_io_util.cc"
#include "zip_open.cc"
#include "zip_progress.cc"
#include "zip_source_buffer.cc"
#include "zip_source_compress.cc"
#include "zip_source.cc"
#include "zip_utf-8.cc"
