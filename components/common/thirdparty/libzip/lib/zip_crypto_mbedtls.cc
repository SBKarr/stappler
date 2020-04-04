/*
  zip_crypto_mbedtls.c -- mbed TLS wrapper
  Copyright (C) 2018-2019 Dieter Baron and Thomas Klausner

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

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/md.h>

typedef struct {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
} zip_random_context_t;

ZIP_EXTERN bool
zip_secure_random(zip_uint8_t *buffer, zip_uint16_t length) {
    static zip_random_context_t *ctx = NULL;
    const char *pers = "zip_crypto_mbedtls";

    if (!ctx) {
	ctx = (zip_random_context_t *)malloc(sizeof(zip_random_context_t));
	if (!ctx) {
	    return false;
	}
	mbedtls_entropy_init(&ctx->entropy);
	mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
	if (mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy, (const unsigned char *)pers, strlen((const char *)pers)) != 0) {
	    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
	    mbedtls_entropy_free(&ctx->entropy);
	    free(ctx);
	    ctx = NULL;
	    return false;
	}
    }

    return mbedtls_ctr_drbg_random(&ctx->ctr_drbg, (unsigned char *)buffer, (size_t)length) == 0;
}
