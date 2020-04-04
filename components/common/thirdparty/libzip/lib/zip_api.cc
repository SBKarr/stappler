/*
  zip.h -- exported declarations.
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

ZIP_EXTERN int zip_delete(zip_t *za, zip_uint64_t idx) {
    const char *name;

    if (idx >= za->nentry) {
	zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	return -1;
    }

    if (ZIP_IS_RDONLY(za)) {
	zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
	return -1;
    }

    if ((name = _zip_get_name(za, idx, 0, &za->error)) == NULL) {
	return -1;
    }

    if (!_zip_hash_delete(za->names, (const zip_uint8_t *)name, &za->error)) {
	return -1;
    }

    /* allow duplicate file names, because the file will
     * be removed directly afterwards */
    if (_zip_unchange(za, idx, 1) != 0)
	return -1;

    za->entry[idx].deleted = 1;

    return 0;
}

ZIP_EXTERN zip_int64_t zip_dir_add(zip_t *za, const char *name, zip_flags_t flags) {
	size_t len;
	zip_int64_t idx;
	char *s;
	zip_source_t *source;

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	if (name == NULL) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	s = NULL;
	len = strlen(name);

	if (name[len - 1] != '/') {
		if ((s = (char*) malloc(len + 2)) == NULL) {
			zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
			return -1;
		}
		strcpy(s, name);
		s[len] = '/';
		s[len + 1] = '\0';
	}

	if ((source = zip_source_buffer(za, NULL, 0, 0)) == NULL) {
		free(s);
		return -1;
	}

	idx = _zip_file_replace(za, ZIP_UINT64_MAX, s ? s : name, source, flags);

	free(s);

	if (idx < 0)
		zip_source_free(source);
	else {
		if (zip_file_set_external_attributes(za, (zip_uint64_t) idx, 0, ZIP_OPSYS_DEFAULT, ZIP_EXT_ATTRIB_DEFAULT_DIR)
				< 0) {
			zip_delete(za, (zip_uint64_t) idx);
			return -1;
		}
	}

	return idx;
}

/* zip_discard:
   frees the space allocated to a zipfile struct, and closes the
   corresponding file. */

void zip_discard(zip_t *za) {
	zip_uint64_t i;

	if (za == NULL)
		return;

	if (za->src) {
		zip_source_close(za->src);
		zip_source_free(za->src);
	}

	free(za->default_password);
	_zip_string_free(za->comment_orig);
	_zip_string_free(za->comment_changes);

	_zip_hash_free(za->names);

	if (za->entry) {
		for (i = 0; i < za->nentry; i++)
			_zip_entry_finalize(za->entry + i);
		free(za->entry);
	}

	for (i = 0; i < za->nopen_source; i++) {
		_zip_source_invalidate(za->open_source[i]);
	}
	free(za->open_source);

	_zip_progress_free(za->progress);

	zip_error_fini(&za->error);

	free(za);

	return;
}

ZIP_EXTERN const char *
zip_get_archive_comment(zip_t *za, int *lenp, zip_flags_t flags) {
    zip_string_t *comment;
    zip_uint32_t len;
    const zip_uint8_t *str;

    if ((flags & ZIP_FL_UNCHANGED) || (za->comment_changes == NULL))
	comment = za->comment_orig;
    else
	comment = za->comment_changes;

    if ((str = _zip_string_get(comment, &len, flags, &za->error)) == NULL)
	return NULL;

    if (lenp)
	*lenp = (int)len;

    return (const char *)str;
}

ZIP_EXTERN int
zip_get_archive_flag(zip_t *za, zip_flags_t flag, zip_flags_t flags) {
    unsigned int fl;

    fl = (flags & ZIP_FL_UNCHANGED) ? za->flags : za->ch_flags;

    return (fl & flag) ? 1 : 0;
}

ZIP_EXTERN int
zip_encryption_method_supported(zip_uint16_t method, int encode) {
    if (method == ZIP_EM_NONE) {
        return 1;
    }
    return 0;
}

ZIP_EXTERN const char *
zip_get_name(zip_t *za, zip_uint64_t idx, zip_flags_t flags) {
    return _zip_get_name(za, idx, flags, &za->error);
}

static const char *
_zip_get_name(zip_t *za, zip_uint64_t idx, zip_flags_t flags, zip_error_t *error) {
    zip_dirent_t *de;
    const zip_uint8_t *str;

    if ((de = _zip_get_dirent(za, idx, flags, error)) == NULL)
	return NULL;

    if ((str = _zip_string_get(de->filename, NULL, flags, error)) == NULL)
	return NULL;

    return (const char *)str;
}

ZIP_EXTERN zip_int64_t
zip_get_num_entries(zip_t *za, zip_flags_t flags) {
	zip_uint64_t n;

	if (za == NULL)
		return -1;

	if (flags & ZIP_FL_UNCHANGED) {
		n = za->nentry;
		while (n > 0 && za->entry[n - 1].orig == NULL)
			--n;
		return (zip_int64_t) n;
	}
	return (zip_int64_t) za->nentry;
}

static void *
_zip_memdup(const void *mem, size_t len, zip_error_t *error) {
	void *ret;

	if (len == 0)
		return NULL;

	ret = malloc(len);
	if (!ret) {
		zip_error_set(error, ZIP_ER_MEMORY, 0);
		return NULL;
	}

	memcpy(ret, mem, len);

	return ret;
}

ZIP_EXTERN zip_int64_t
zip_name_locate(zip_t *za, const char *fname, zip_flags_t flags) {
    return _zip_name_locate(za, fname, flags, &za->error);
}


static zip_int64_t
_zip_name_locate(zip_t *za, const char *fname, zip_flags_t flags, zip_error_t *error) {
	int (*cmp)(const char*, const char*);
	const char *fn, *p;
	zip_uint64_t i;

	if (za == NULL)
		return -1;

	if (fname == NULL) {
		zip_error_set(error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (flags & (ZIP_FL_NOCASE | ZIP_FL_NODIR | ZIP_FL_ENC_CP437)) {
		/* can't use hash table */
		cmp = (flags & ZIP_FL_NOCASE) ? strcasecmp : strcmp;

		for (i = 0; i < za->nentry; i++) {
			fn = _zip_get_name(za, i, flags, error);

			/* newly added (partially filled) entry or error */
			if (fn == NULL)
				continue;

			if (flags & ZIP_FL_NODIR) {
				p = strrchr(fn, '/');
				if (p)
					fn = p + 1;
			}

			if (cmp(fname, fn) == 0) {
				_zip_error_clear(error);
				return (zip_int64_t) i;
			}
		}

		zip_error_set(error, ZIP_ER_NOENT, 0);
		return -1;
	} else {
		return _zip_hash_lookup(za->names, (const zip_uint8_t*) fname, flags, error);
	}
}

/* _zip_new:
   creates a new zipfile struct, and sets the contents to zero; returns
   the new struct. */

static zip_t *
_zip_new(zip_error_t *error) {
	zip_t *za;

	za = (zip_t*) malloc(sizeof(struct zip));
	if (!za) {
		zip_error_set(error, ZIP_ER_MEMORY, 0);
		return NULL;
	}

	if ((za->names = _zip_hash_new(error)) == NULL) {
		free(za);
		return NULL;
	}

	za->src = NULL;
	za->open_flags = 0;
	zip_error_init(&za->error);
	za->flags = za->ch_flags = 0;
	za->default_password = NULL;
	za->comment_orig = za->comment_changes = NULL;
	za->comment_changed = 0;
	za->nentry = za->nentry_alloc = 0;
	za->entry = NULL;
	za->nopen_source = za->nopen_source_alloc = 0;
	za->open_source = NULL;
	za->progress = NULL;

	return za;
}

ZIP_EXTERN int
zip_set_archive_comment(zip_t *za, const char *comment, zip_uint16_t len) {
	zip_string_t *cstr;

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	if (len > 0 && comment == NULL) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (len > 0) {
		if ((cstr = _zip_string_new((const zip_uint8_t*) comment, len, ZIP_FL_ENC_GUESS, &za->error)) == NULL)
			return -1;

		if (_zip_guess_encoding(cstr, ZIP_ENCODING_UNKNOWN) == ZIP_ENCODING_CP437) {
			_zip_string_free(cstr);
			zip_error_set(&za->error, ZIP_ER_INVAL, 0);
			return -1;
		}
	} else
		cstr = NULL;

	_zip_string_free(za->comment_changes);
	za->comment_changes = NULL;

	if (((za->comment_orig && _zip_string_equal(za->comment_orig, cstr)) || (za->comment_orig == NULL && cstr == NULL))) {
		_zip_string_free(cstr);
		za->comment_changed = 0;
	} else {
		za->comment_changes = cstr;
		za->comment_changed = 1;
	}

    return 0;
}

ZIP_EXTERN int
zip_set_archive_flag(zip_t *za, zip_flags_t flag, int value) {
	unsigned int new_flags;

	if (value)
		new_flags = za->ch_flags | flag;
	else
		new_flags = za->ch_flags & ~flag;

	if (new_flags == za->ch_flags)
		return 0;

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	if ((flag & ZIP_AFL_RDONLY) && value && (za->ch_flags & ZIP_AFL_RDONLY) == 0) {
		if (_zip_changed(za, NULL)) {
			zip_error_set(&za->error, ZIP_ER_CHANGED, 0);
			return -1;
		}
	}

	za->ch_flags = new_flags;

	return 0;
}

ZIP_EXTERN int
zip_set_default_password(zip_t *za, const char *passwd) {
	if (za == NULL)
		return -1;

	free(za->default_password);

	if (passwd) {
		if ((za->default_password = strdup(passwd)) == NULL) {
			zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
			return -1;
		}
	} else
		za->default_password = NULL;

	return 0;
}

ZIP_EXTERN int
zip_set_file_compression(zip_t *za, zip_uint64_t idx, zip_int32_t method, zip_uint32_t flags) {
	zip_entry_t *e;
	zip_int32_t old_method;

	if (idx >= za->nentry || flags > 9) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	if (!zip_compression_method_supported(method, true)) {
		zip_error_set(&za->error, ZIP_ER_COMPNOTSUPP, 0);
		return -1;
	}

	e = za->entry + idx;

	old_method = (e->orig == NULL ? ZIP_CM_DEFAULT : e->orig->comp_method);

	/* TODO: do we want to recompress if level is set? Only if it's
	 * different than what bit flags tell us, but those are not
	 * defined for all compression methods, or not directly mappable
	 * to levels */

	if (method == old_method) {
		if (e->changes) {
			e->changes->changed &= ~ZIP_DIRENT_COMP_METHOD;
			e->changes->compression_level = 0;
			if (e->changes->changed == 0) {
				_zip_dirent_free(e->changes);
				e->changes = NULL;
			}
		}
	} else {
		if (e->changes == NULL) {
			if ((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
				zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
				return -1;
			}
		}

		e->changes->comp_method = method;
		e->changes->compression_level = (zip_uint16_t) flags;
		e->changes->changed |= ZIP_DIRENT_COMP_METHOD;
	}

	return 0;
}

static int
_zip_set_name(zip_t *za, zip_uint64_t idx, const char *name, zip_flags_t flags) {
	zip_entry_t *e;
	zip_string_t *str;
	bool same_as_orig;
	zip_int64_t i;
	const zip_uint8_t *old_name, *new_name;
	zip_string_t *old_str;

	if (idx >= za->nentry) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, ZIP_ER_RDONLY, 0);
		return -1;
	}

	if (name && name[0] != '\0') {
		/* TODO: check for string too long */
		if ((str = _zip_string_new((const zip_uint8_t*) name, (zip_uint16_t) strlen(name), flags, &za->error)) == NULL)
			return -1;
		if ((flags & ZIP_FL_ENCODING_ALL) == ZIP_FL_ENC_GUESS
				&& _zip_guess_encoding(str, ZIP_ENCODING_UNKNOWN) == ZIP_ENCODING_UTF8_GUESSED)
			str->encoding = ZIP_ENCODING_UTF8_KNOWN;
	} else
		str = NULL;

	/* TODO: encoding flags needed for CP437? */
	if ((i = _zip_name_locate(za, name, 0, NULL)) >= 0 && (zip_uint64_t) i != idx) {
		_zip_string_free(str);
		zip_error_set(&za->error, ZIP_ER_EXISTS, 0);
		return -1;
	}

	/* no effective name change */
	if (i >= 0 && (zip_uint64_t) i == idx) {
		_zip_string_free(str);
		return 0;
	}

	e = za->entry + idx;

	if (e->orig)
		same_as_orig = _zip_string_equal(e->orig->filename, str);
	else
		same_as_orig = false;

	if (!same_as_orig && e->changes == NULL) {
		if ((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
			zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
			_zip_string_free(str);
			return -1;
		}
	}

	if ((new_name = _zip_string_get(same_as_orig ? e->orig->filename : str, NULL, 0, &za->error)) == NULL) {
		_zip_string_free(str);
		return -1;
	}

	if (e->changes) {
		old_str = e->changes->filename;
	} else if (e->orig) {
		old_str = e->orig->filename;
	} else {
		old_str = NULL;
	}

	if (old_str) {
		if ((old_name = _zip_string_get(old_str, NULL, 0, &za->error)) == NULL) {
			_zip_string_free(str);
			return -1;
		}
	} else {
		old_name = NULL;
	}

	if (_zip_hash_add(za->names, new_name, idx, 0, &za->error) == false) {
		_zip_string_free(str);
		return -1;
	}
	if (old_name) {
		_zip_hash_delete(za->names, old_name, NULL);
	}

	if (same_as_orig) {
		if (e->changes) {
			if (e->changes->changed & ZIP_DIRENT_FILENAME) {
				_zip_string_free(e->changes->filename);
				e->changes->changed &= ~ZIP_DIRENT_FILENAME;
				if (e->changes->changed == 0) {
					_zip_dirent_free(e->changes);
					e->changes = NULL;
				} else {
					/* TODO: what if not cloned? can that happen? */
					e->changes->filename = e->orig->filename;
				}
			}
		}
		_zip_string_free(str);
	} else {
		if (e->changes->changed & ZIP_DIRENT_FILENAME) {
			_zip_string_free(e->changes->filename);
		}
		e->changes->changed |= ZIP_DIRENT_FILENAME;
		e->changes->filename = str;
	}

	return 0;
}

ZIP_EXTERN int
zip_stat_index(zip_t *za, zip_uint64_t index, zip_flags_t flags, zip_stat_t *st) {
	const char *name;
	zip_dirent_t *de;

	if ((de = _zip_get_dirent(za, index, flags, NULL)) == NULL)
		return -1;

	if ((name = zip_get_name(za, index, flags)) == NULL)
		return -1;

	if ((flags & ZIP_FL_UNCHANGED) == 0 && ZIP_ENTRY_DATA_CHANGED(za->entry + index)) {
		zip_entry_t *entry = za->entry + index;

		if (zip_source_stat(entry->source, st) < 0) {
			zip_error_set(&za->error, ZIP_ER_CHANGED, 0);
			return -1;
		}

		if (entry->changes->changed & ZIP_DIRENT_LAST_MOD) {
			st->mtime = de->last_mod;
			st->valid |= ZIP_STAT_MTIME;
		}
	} else {
		zip_stat_init(st);

		st->crc = de->crc;
		st->size = de->uncomp_size;
		st->mtime = de->last_mod;
		st->comp_size = de->comp_size;
		st->comp_method = (zip_uint16_t) de->comp_method;
		st->encryption_method = de->encryption_method;
		st->valid = (de->crc_valid ? ZIP_STAT_CRC : 0) | ZIP_STAT_SIZE | ZIP_STAT_MTIME | ZIP_STAT_COMP_SIZE
				| ZIP_STAT_COMP_METHOD | ZIP_STAT_ENCRYPTION_METHOD;
	}

	st->index = index;
	st->name = name;
	st->valid |= ZIP_STAT_INDEX | ZIP_STAT_NAME;

	return 0;
}

ZIP_EXTERN void
zip_stat_init(zip_stat_t *st) {
    st->valid = 0;
    st->name = NULL;
    st->index = ZIP_UINT64_MAX;
    st->crc = 0;
    st->mtime = (time_t)-1;
    st->size = 0;
    st->comp_size = 0;
    st->comp_method = ZIP_CM_STORE;
    st->encryption_method = ZIP_EM_NONE;
}

static int
_zip_stat_merge(zip_stat_t *dst, const zip_stat_t *src, zip_error_t *error) {
	/* name is not merged, since zip_stat_t doesn't own it, and src may not be valid as long as dst */
	if (src->valid & ZIP_STAT_INDEX) {
		dst->index = src->index;
	}
	if (src->valid & ZIP_STAT_SIZE) {
		dst->size = src->size;
	}
	if (src->valid & ZIP_STAT_COMP_SIZE) {
		dst->comp_size = src->comp_size;
	}
	if (src->valid & ZIP_STAT_MTIME) {
		dst->mtime = src->mtime;
	}
	if (src->valid & ZIP_STAT_CRC) {
		dst->crc = src->crc;
	}
	if (src->valid & ZIP_STAT_COMP_METHOD) {
		dst->comp_method = src->comp_method;
	}
	if (src->valid & ZIP_STAT_ENCRYPTION_METHOD) {
		dst->encryption_method = src->encryption_method;
	}
	if (src->valid & ZIP_STAT_FLAGS) {
		dst->flags = src->flags;
	}
	dst->valid |= src->valid;

	return 0;
}

ZIP_EXTERN int
zip_stat(zip_t *za, const char *fname, zip_flags_t flags, zip_stat_t *st) {
	zip_int64_t idx;

	if ((idx = zip_name_locate(za, fname, flags)) < 0)
		return -1;

	return zip_stat_index(za, (zip_uint64_t) idx, flags, st);
}

ZIP_EXTERN const char *
zip_strerror(zip_t *za) {
    return zip_error_strerror(&za->error);
}

ZIP_EXTERN int
zip_unchange_all(zip_t *za) {
	int ret;
	zip_uint64_t i;

	if (!_zip_hash_revert(za->names, &za->error)) {
		return -1;
	}

	ret = 0;
	for (i = 0; i < za->nentry; i++)
		ret |= _zip_unchange(za, i, 1);

	ret |= zip_unchange_archive(za);

	return ret;
}

ZIP_EXTERN int
zip_unchange_archive(zip_t *za) {
    if (za->comment_changed) {
	_zip_string_free(za->comment_changes);
	za->comment_changes = NULL;
	za->comment_changed = 0;
    }

    za->ch_flags = za->flags;

    return 0;
}

static void
_zip_unchange_data(zip_entry_t *ze) {
	if (ze->source) {
		zip_source_free(ze->source);
		ze->source = NULL;
	}

	if (ze->changes != NULL && (ze->changes->changed & ZIP_DIRENT_COMP_METHOD)
			&& ze->changes->comp_method == ZIP_CM_REPLACED_DEFAULT) {
		ze->changes->changed &= ~ZIP_DIRENT_COMP_METHOD;
		if (ze->changes->changed == 0) {
			_zip_dirent_free(ze->changes);
			ze->changes = NULL;
		}
	}

	ze->deleted = 0;
}

ZIP_EXTERN int
zip_unchange(zip_t *za, zip_uint64_t idx) {
    return _zip_unchange(za, idx, 0);
}


static int
_zip_unchange(zip_t *za, zip_uint64_t idx, int allow_duplicates) {
	zip_int64_t i;
	const char *orig_name, *changed_name;

	if (idx >= za->nentry) {
		zip_error_set(&za->error, ZIP_ER_INVAL, 0);
		return -1;
	}

	if (!allow_duplicates && za->entry[idx].changes && (za->entry[idx].changes->changed & ZIP_DIRENT_FILENAME)) {
		if (za->entry[idx].orig != NULL) {
			if ((orig_name = _zip_get_name(za, idx, ZIP_FL_UNCHANGED, &za->error)) == NULL) {
				return -1;
			}

			i = _zip_name_locate(za, orig_name, 0, NULL);
			if (i >= 0 && (zip_uint64_t) i != idx) {
				zip_error_set(&za->error, ZIP_ER_EXISTS, 0);
				return -1;
			}
		} else {
			orig_name = NULL;
		}

		if ((changed_name = _zip_get_name(za, idx, 0, &za->error)) == NULL) {
			return -1;
		}

		if (orig_name) {
			if (_zip_hash_add(za->names, (const zip_uint8_t*) orig_name, idx, 0, &za->error) == false) {
				return -1;
			}
		}
		if (_zip_hash_delete(za->names, (const zip_uint8_t*) changed_name, &za->error) == false) {
			_zip_hash_delete(za->names, (const zip_uint8_t*) orig_name, NULL);
			return -1;
		}
	}

	_zip_dirent_free(za->entry[idx].changes);
	za->entry[idx].changes = NULL;

	_zip_unchange_data(za->entry + idx);

	return 0;
}

static void
_zip_source_invalidate(zip_source_t *src) {
	src->source_closed = 1;

	if (zip_error_code_zip(&src->error) == ZIP_ER_OK) {
		zip_error_set(&src->error, ZIP_ER_ZIPCLOSED, 0);
	}
}
