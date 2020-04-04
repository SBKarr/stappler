/*
  zip_file.c -- get offset of file data in archive.
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

ZIP_EXTERN int
zip_fclose(zip_file_t *zf) {
    int ret;

    if (zf->src)
	zip_source_free(zf->src);

    ret = 0;
    if (zf->error.zip_err)
	ret = zf->error.zip_err;

    zip_error_fini(&zf->error);
    free(zf);
    return ret;
}

/*
  NOTE: Return type is signed so we can return -1 on error.
	The index can not be larger than ZIP_INT64_MAX since the size
	of the central directory cannot be larger than
	ZIP_UINT64_MAX, and each entry is larger than 2 bytes.
*/

ZIP_EXTERN zip_int64_t
zip_file_add(zip_t *za, const char *name, zip_source_t *source, zip_flags_t flags) {
    if (name == NULL || source == NULL) {
	zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	return -1;
    }

    return _zip_file_replace(za, ZIP_UINT64_MAX, name, source, flags);
}

ZIP_EXTERN void
zip_file_error_clear(zip_file_t *zf) {
	if (zf == NULL)
		return;

	_zip_error_clear(&zf->error);
}

/* lenp is 32 bit because converted comment can be longer than ZIP_UINT16_MAX */

ZIP_EXTERN const char*
zip_file_get_comment(zip_t *za, zip_uint64_t idx, zip_uint32_t *lenp, zip_flags_t flags) {
	zip_dirent_t *de;
	zip_uint32_t len;
	const zip_uint8_t *str;

	if ((de = _zip_get_dirent(za, idx, flags, NULL)) == NULL)
		return NULL;

	if ((str = _zip_string_get(de->comment, &len, flags, &za->error)) == NULL)
		return NULL;

	if (lenp)
		*lenp = len;

	return (const char*) str;
}

int
zip_file_get_external_attributes(zip_t *za, zip_uint64_t idx, zip_flags_t flags, zip_uint8_t *opsys, zip_uint32_t *attributes) {
	zip_dirent_t *de;

	if ((de = _zip_get_dirent(za, idx, flags, NULL)) == NULL)
		return -1;

	if (opsys)
		*opsys = (zip_uint8_t) ((de->version_madeby >> 8) & 0xff);

	if (attributes)
		*attributes = de->ext_attrib;

	return 0;
}

/* _zip_file_get_offset(za, ze):
   Returns the offset of the file data for entry ze.

   On error, fills in za->error and returns 0.
*/

zip_uint64_t
_zip_file_get_offset(const zip_t *za, zip_uint64_t idx, zip_error_t *error) {
    zip_uint64_t offset;
    zip_int32_t size;

    if (za->entry[idx].orig == NULL) {
	zip_error_set(error, ZIP_ER_INTERNAL, 0);
	return 0;
    }

    offset = za->entry[idx].orig->offset;

    if (zip_source_seek(za->src, (zip_int64_t)offset, SEEK_SET) < 0) {
	_zip_error_set_from_source(error, za->src);
	return 0;
    }

    /* TODO: cache? */
    if ((size = _zip_dirent_size(za->src, ZIP_EF_LOCAL, error)) < 0)
	return 0;

    if (offset + (zip_uint32_t)size > ZIP_INT64_MAX) {
	zip_error_set(error, ZIP_ER_SEEK, EFBIG);
	return 0;
    }

    return offset + (zip_uint32_t)size;
}

zip_uint64_t
_zip_file_get_end(const zip_t *za, zip_uint64_t index, zip_error_t *error) {
    zip_uint64_t offset;
    zip_dirent_t *entry;

    if ((offset = _zip_file_get_offset(za, index, error)) == 0) {
	return 0;
    }

    entry = za->entry[index].orig;

    if (offset + entry->comp_size < offset || offset + entry->comp_size > ZIP_INT64_MAX) {
	zip_error_set(error, ZIP_ER_SEEK, EFBIG);
	return 0;
    }
    offset += entry->comp_size;

    if (entry->bitflags & ZIP_GPBF_DATA_DESCRIPTOR) {
	zip_uint8_t buf[4];
	if (zip_source_seek(za->src, (zip_int64_t)offset, SEEK_SET) < 0) {
	    _zip_error_set_from_source(error, za->src);
	    return 0;
	}
	if (zip_source_read(za->src, buf, 4) != 4) {
	    _zip_error_set_from_source(error, za->src);
	    return 0;
	}
	if (memcmp(buf, DATADES_MAGIC, 4) == 0) {
	    offset += 4;
	}
	offset += 12;
	if (_zip_dirent_needs_zip64(entry, 0)) {
	    offset += 8;
	}
	if (offset > ZIP_INT64_MAX) {
	    zip_error_set(error, ZIP_ER_SEEK, EFBIG);
	    return 0;
	}
    }

    return offset;
}

ZIP_EXTERN int
zip_file_rename(zip_t *za, zip_uint64_t idx, const char *name, zip_flags_t flags) {
	const char *old_name;
	int old_is_dir, new_is_dir;

	if (idx >= za->nentry || (name != NULL && strlen(name) > ZIP_UINT16_MAX)) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	if ((old_name = zip_get_name(za, idx, 0)) == NULL)
		return -1;

	new_is_dir = (name != NULL && name[strlen(name) - 1] == '/');
	old_is_dir = (old_name[strlen(old_name) - 1] == '/');

	if (new_is_dir != old_is_dir) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	return _zip_set_name(za, idx, name, flags);
}

ZIP_EXTERN int
zip_file_replace(zip_t *za, zip_uint64_t idx, zip_source_t *source, zip_flags_t flags) {
    if (idx >= za->nentry || source == NULL) {
	zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	return -1;
    }

    if (_zip_file_replace(za, idx, NULL, source, flags) == -1)
	return -1;

    return 0;
}


/* NOTE: Signed due to -1 on error.  See zip_add.c for more details. */

zip_int64_t
_zip_file_replace(zip_t *za, zip_uint64_t idx, const char *name, zip_source_t *source, zip_flags_t flags) {
	zip_uint64_t za_nentry_prev;

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	za_nentry_prev = za->nentry;
	if (idx == ZIP_UINT64_MAX) {
		zip_int64_t i = -1;

		if (flags & ZIP_FL_OVERWRITE)
			i = _zip_name_locate(za, name, flags, NULL);

		if (i == -1) {
			/* create and use new entry, used by zip_add */
			if ((i = _zip_add_entry(za)) < 0)
				return -1;
		}
		idx = (zip_uint64_t) i;
	}

	if (name && _zip_set_name(za, idx, name, flags) != 0) {
		if (za->nentry != za_nentry_prev) {
			_zip_entry_finalize(za->entry + idx);
			za->nentry = za_nentry_prev;
		}
		return -1;
	}

	/* does not change any name related data, so we can do it here;
	 * needed for a double add of the same file name */
	_zip_unchange_data(za->entry + idx);

	if (za->entry[idx].orig != NULL
			&& (za->entry[idx].changes == NULL || (za->entry[idx].changes->changed & ZIP_DIRENT_COMP_METHOD) == 0)) {
		if (za->entry[idx].changes == NULL) {
			if ((za->entry[idx].changes = _zip_dirent_clone(za->entry[idx].orig)) == NULL) {
				zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
				return -1;
			}
		}

		za->entry[idx].changes->comp_method = ZIP_CM_REPLACED_DEFAULT;
		za->entry[idx].changes->changed |= ZIP_DIRENT_COMP_METHOD;
	}

	za->entry[idx].source = source;

	return (zip_int64_t) idx;
}

ZIP_EXTERN int
zip_file_set_comment(zip_t *za, zip_uint64_t idx, const char *comment, zip_uint16_t len, zip_flags_t flags) {
	zip_entry_t *e;
	zip_string_t *cstr;
	int changed;

	if (_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	if (len > 0 && comment == NULL) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (len > 0) {
		if ((cstr = _zip_string_new((const zip_uint8_t*) comment, len, flags, &za->error)) == NULL)
			return -1;
		if ((flags & ZIP_FL_ENCODING_ALL) == ZIP_FL_ENC_GUESS
				&& _zip_guess_encoding(cstr, ZIP_ENCODING_UNKNOWN) == ZIP_ENCODING_UTF8_GUESSED)
			cstr->encoding = ZIP_ENCODING_UTF8_KNOWN;
	} else
		cstr = NULL;

	e = za->entry + idx;

	if (e->changes) {
		_zip_string_free(e->changes->comment);
		e->changes->comment = NULL;
		e->changes->changed &= ~ZIP_DIRENT_COMMENT;
	}

	if (e->orig && e->orig->comment)
		changed = !_zip_string_equal(e->orig->comment, cstr);
	else
		changed = (cstr != NULL);

	if (changed) {
		if (e->changes == NULL) {
			if ((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
				zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
				_zip_string_free(cstr);
				return -1;
			}
		}
		e->changes->comment = cstr;
		e->changes->changed |= ZIP_DIRENT_COMMENT;
	} else {
		_zip_string_free(cstr);
		if (e->changes && e->changes->changed == 0) {
			_zip_dirent_free(e->changes);
			e->changes = NULL;
		}
	}

	return 0;
}

ZIP_EXTERN int
zip_file_set_encryption(zip_t *za, zip_uint64_t idx, zip_uint16_t method, const char *password) {
	zip_entry_t *e;
	zip_uint16_t old_method;

	if (idx >= za->nentry) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	if (method != ZIP_EM_NONE) {
		zip_error_set(&za->error, ZIP_ER_ENCRNOTSUPP, 0);
		return -1;
	}

	e = za->entry + idx;

	old_method = (e->orig == NULL ? ZIP_EM_NONE : e->orig->encryption_method);

	if (method == old_method && password == NULL) {
		if (e->changes) {
			if (e->changes->changed & ZIP_DIRENT_PASSWORD) {
				_zip_crypto_clear(e->changes->password, strlen(e->changes->password));
				free(e->changes->password);
				e->changes->password = (e->orig == NULL ? NULL : e->orig->password);
			}
			e->changes->changed &= ~(ZIP_DIRENT_ENCRYPTION_METHOD | ZIP_DIRENT_PASSWORD);
			if (e->changes->changed == 0) {
				_zip_dirent_free(e->changes);
				e->changes = NULL;
			}
		}
	} else {
		char *our_password = NULL;

		if (password) {
			if ((our_password = strdup(password)) == NULL) {
				zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
				return -1;
			}
		}

		if (e->changes == NULL) {
			if ((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
				if (our_password) {
					_zip_crypto_clear(our_password, strlen(our_password));
				}
				free(our_password);
				zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
				return -1;
			}
		}

		e->changes->encryption_method = method;
		e->changes->changed |= ZIP_DIRENT_ENCRYPTION_METHOD;
		if (password) {
			e->changes->password = our_password;
			e->changes->changed |= ZIP_DIRENT_PASSWORD;
		} else {
			if (e->changes->changed & ZIP_DIRENT_PASSWORD) {
				_zip_crypto_clear(e->changes->password, strlen(e->changes->password));
				free(e->changes->password);
				e->changes->password = e->orig ? e->orig->password : NULL;
				e->changes->changed &= ~ZIP_DIRENT_PASSWORD;
			}
		}
	}

	return 0;
}

ZIP_EXTERN int
zip_file_set_external_attributes(zip_t *za, zip_uint64_t idx, zip_flags_t flags, zip_uint8_t opsys, zip_uint32_t attributes) {
	zip_entry_t *e;
	int changed;
	zip_uint8_t unchanged_opsys;
	zip_uint32_t unchanged_attributes;

	if (_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	e = za->entry + idx;

	unchanged_opsys = (e->orig ? (zip_uint8_t) (e->orig->version_madeby >> 8) : (zip_uint8_t) ZIP_OPSYS_DEFAULT);
	unchanged_attributes = e->orig ? e->orig->ext_attrib : ZIP_EXT_ATTRIB_DEFAULT;

	changed = (opsys != unchanged_opsys || attributes != unchanged_attributes);

	if (changed) {
		if (e->changes == NULL) {
			if ((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
				zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
				return -1;
			}
		}
		e->changes->version_madeby = (zip_uint16_t) ((opsys << 8) | (e->changes->version_madeby & 0xff));
		e->changes->ext_attrib = attributes;
		e->changes->changed |= ZIP_DIRENT_ATTRIBUTES;
	} else if (e->changes) {
		e->changes->changed &= ~ZIP_DIRENT_ATTRIBUTES;
		if (e->changes->changed == 0) {
			_zip_dirent_free(e->changes);
			e->changes = NULL;
		} else {
			e->changes->version_madeby = (zip_uint16_t) ((unchanged_opsys << 8) | (e->changes->version_madeby & 0xff));
			e->changes->ext_attrib = unchanged_attributes;
		}
	}

	return 0;
}

ZIP_EXTERN int
zip_file_set_dostime(zip_t *za, zip_uint64_t idx, zip_uint16_t dtime, zip_uint16_t ddate, zip_flags_t flags) {
    time_t mtime;
    mtime = _zip_d2u_time(dtime, ddate);
    return zip_file_set_mtime(za, idx, mtime, flags);
}

ZIP_EXTERN int
zip_file_set_mtime(zip_t *za, zip_uint64_t idx, time_t mtime, zip_flags_t flags) {
	zip_entry_t *e;

	if (_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	e = za->entry + idx;

	if (e->changes == NULL) {
		if ((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
			zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
			return -1;
		}
	}

	e->changes->last_mod = mtime;
	e->changes->changed |= ZIP_DIRENT_LAST_MOD;

	return 0;
}

ZIP_EXTERN const char *
zip_file_strerror(zip_file_t *zf) {
    return zip_error_strerror(&zf->error);
}

ZIP_EXTERN zip_file_t *
zip_fopen_encrypted(zip_t *za, const char *fname, zip_flags_t flags, const char *password) {
    zip_int64_t idx;

    if ((idx = zip_name_locate(za, fname, flags)) < 0)
	return NULL;

    return zip_fopen_index_encrypted(za, (zip_uint64_t)idx, flags, password);
}

static zip_file_t *_zip_file_new(zip_t *za);

ZIP_EXTERN zip_file_t *
zip_fopen_index_encrypted(zip_t *za, zip_uint64_t index, zip_flags_t flags, const char *password) {
	zip_file_t *zf;
	zip_source_t *src;

	if ((src = _zip_source_zip_new(za, za, index, flags, 0, 0, password)) == NULL)
		return NULL;

	if (zip_source_open(src) < 0) {
		_zip_error_set_from_source(&za->error, src);
		zip_source_free(src);
		return NULL;
	}

	if ((zf = _zip_file_new(za)) == NULL) {
		zip_source_free(src);
		return NULL;
	}

	zf->src = src;

	return zf;
}

static zip_file_t *
_zip_file_new(zip_t *za) {
	zip_file_t *zf;

	if ((zf = (zip_file_t*) malloc(sizeof(struct zip_file))) == NULL) {
		zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
		return NULL;
	}

	zf->za = za;
	zip_error_init(&zf->error);
	zf->eof = 0;
	zf->src = NULL;

	return zf;
}

ZIP_EXTERN zip_file_t *
zip_fopen_index(zip_t *za, zip_uint64_t index, zip_flags_t flags) {
    return zip_fopen_index_encrypted(za, index, flags, za->default_password);
}

ZIP_EXTERN zip_file_t *
zip_fopen(zip_t *za, const char *fname, zip_flags_t flags) {
    zip_int64_t idx;

    if ((idx = zip_name_locate(za, fname, flags)) < 0)
	return NULL;

    return zip_fopen_index_encrypted(za, (zip_uint64_t)idx, flags, za->default_password);
}

ZIP_EXTERN zip_int64_t
zip_fread(zip_file_t *zf, void *outbuf, zip_uint64_t toread) {
	zip_int64_t n;

	if (!zf)
		return -1;

	if (zf->error.zip_err != 0)
		return -1;

	if (toread > ZIP_INT64_MAX) {
		zip_error_set(&zf->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if ((zf->eof) || (toread == 0))
		return 0;

	if ((n = zip_source_read(zf->src, outbuf, toread)) < 0) {
		_zip_error_set_from_source(&zf->error, zf->src);
		return -1;
	}

	return n;
}

ZIP_EXTERN zip_int8_t
zip_fseek(zip_file_t *zf, zip_int64_t offset, int whence) {
	if (!zf)
		return -1;

	if (zf->error.zip_err != 0)
		return -1;

	if (zip_source_seek(zf->src, offset, whence) < 0) {
		_zip_error_set_from_source(&zf->error, zf->src);
		return -1;
	}

	return 0;
}

ZIP_EXTERN zip_int64_t
zip_ftell(zip_file_t *zf) {
	zip_int64_t res;

	if (!zf)
		return -1;

	if (zf->error.zip_err != 0)
		return -1;

	res = zip_source_tell(zf->src);
	if (res < 0) {
		_zip_error_set_from_source(&zf->error, zf->src);
		return -1;
	}

	return res;
}
