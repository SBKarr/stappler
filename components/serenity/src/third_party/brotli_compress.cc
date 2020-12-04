/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// original mod_brotli can be found in Apache HTTPD sources

#include "httpd.h"
#include "http_core.h"
#include "http_log.h"
#include "apr_strings.h"
#include "Server.h"
#include "Request.h"

#include <brotli/encode.h>

NS_SA_BEGIN

struct brotli_ctx_t {
    BrotliEncoderState *state;
    apr_bucket_brigade *bb;
    apr_off_t total_in;
    apr_off_t total_out;
};

static void *alloc_func(void *opaque, size_t size) {
    return apr_bucket_alloc(size, (apr_bucket_alloc_t *)opaque);
}

static void free_func(void *opaque, void *block) {
    if (block) {
        apr_bucket_free(block);
    }
}

static apr_status_t cleanup_ctx(void *data) {
    brotli_ctx_t *ctx = (brotli_ctx_t *)data;

    BrotliEncoderDestroyInstance(ctx->state);
    ctx->state = NULL;
    return APR_SUCCESS;
}

static brotli_ctx_t *create_ctx(int quality, int lgwin, int lgblock, apr_bucket_alloc_t *alloc, apr_pool_t *pool) {
    brotli_ctx_t *ctx = (brotli_ctx_t *)apr_pcalloc(pool, sizeof(*ctx));

    ctx->state = BrotliEncoderCreateInstance(alloc_func, free_func, alloc);
    BrotliEncoderSetParameter(ctx->state, BROTLI_PARAM_QUALITY, quality);
    BrotliEncoderSetParameter(ctx->state, BROTLI_PARAM_LGWIN, lgwin);
    BrotliEncoderSetParameter(ctx->state, BROTLI_PARAM_LGBLOCK, lgblock);
    apr_pool_cleanup_register(pool, ctx, cleanup_ctx, apr_pool_cleanup_null);

    ctx->bb = apr_brigade_create(pool, alloc);
    ctx->total_in = 0;
    ctx->total_out = 0;

    return ctx;
}

static apr_status_t process_chunk(brotli_ctx_t *ctx, const void *data, apr_size_t len, ap_filter_t *f) {
	const uint8_t *next_in = (const uint8_t *)data;
	apr_size_t avail_in = len;

	while (avail_in > 0) {
		uint8_t *next_out = NULL;
		apr_size_t avail_out = 0;

		if (!BrotliEncoderCompressStream(ctx->state, BROTLI_OPERATION_PROCESS,
				&avail_in, &next_in, &avail_out, &next_out, NULL)) {
			log::text("Brotli", "Error while compressing data");
			return APR_EGENERAL;
		}

		if (BrotliEncoderHasMoreOutput(ctx->state)) {
			apr_size_t output_len = 0;
			const uint8_t *output;
			apr_status_t rv;
			apr_bucket *b;

			/* Drain the accumulated output.  Avoid copying the data by
			 * wrapping a pointer to the internal output buffer and passing
			 * it down to the next filter.  The pointer is only valid until
			 * the next call to BrotliEncoderCompressStream(), but we're okay
			 * with that, since the brigade is cleaned up right after the
			 * ap_pass_brigade() call.
			 */
			output = BrotliEncoderTakeOutput(ctx->state, &output_len);
			ctx->total_out += output_len;

			b = apr_bucket_transient_create((const char *)output, output_len, ctx->bb->bucket_alloc);
			APR_BRIGADE_INSERT_TAIL(ctx->bb, b);

			rv = ap_pass_brigade(f->next, ctx->bb);
			apr_brigade_cleanup(ctx->bb);
			if (rv != APR_SUCCESS) {
				return rv;
			}
		}
	}

	ctx->total_in += len;
	return APR_SUCCESS;
}

static apr_status_t flush(brotli_ctx_t *ctx, BrotliEncoderOperation op, ap_filter_t *f) {
	while (1) {
		const uint8_t *next_in = NULL;
		apr_size_t avail_in = 0;
		uint8_t *next_out = NULL;
		apr_size_t avail_out = 0;
		apr_size_t output_len;
		const uint8_t *output;
		apr_bucket *b;

		if (!BrotliEncoderCompressStream(ctx->state, op, &avail_in, &next_in, &avail_out, &next_out, NULL)) {
			log::text("Brotli", "Error while compressing data");
			return APR_EGENERAL;
		}

		if (!BrotliEncoderHasMoreOutput(ctx->state)) {
			break;
		}

		/* A flush can require several calls to BrotliEncoderCompressStream(),
		 * so place the data on the heap (otherwise, the pointer will become
		 * invalid after the next call to BrotliEncoderCompressStream()).
		 */
		output_len = 0;
		output = BrotliEncoderTakeOutput(ctx->state, &output_len);
		ctx->total_out += output_len;

		b = apr_bucket_heap_create((const char *)output, output_len, NULL, ctx->bb->bucket_alloc);
		APR_BRIGADE_INSERT_TAIL(ctx->bb, b);
	}

	return APR_SUCCESS;
}

static apr_status_t compress_filter(ap_filter_t *f, apr_bucket_brigade *bb) {
	request_rec *r = f->r;
	Request rctx(r);

	brotli_ctx_t *ctx = (brotli_ctx_t *)f->ctx;
	apr_status_t rv;
	Server::CompressionConfig *conf = rctx.server().getCompressionConfig();

	if (APR_BRIGADE_EMPTY(bb)) {
		return APR_SUCCESS;
	}

	if (!ctx) {
		/* Only work on main request, not subrequests, that are not
		 * a 204 response with no content, and are not tagged with the
		 * no-brotli env variable, and are not a partial response to
		 * a Range request.
		 */
		if (r->main || r->status == HTTP_NO_CONTENT || r->status >= HTTP_BAD_REQUEST
				|| apr_table_get(r->subprocess_env, "no-brotli")
				|| apr_table_get(r->headers_out, "Content-Range")) {
			ap_remove_output_filter(f);
			return ap_pass_brigade(f->next, bb);
		}

		if (auto e = apr_table_get(r->headers_out, "Content-Encoding")) {
			auto enc = StringView(e);
			if (enc == "gzip" || enc == "br") {
				// Already encoded, drop
				ap_remove_output_filter(f);
				return ap_pass_brigade(f->next, bb);
			}
		}

		/* Let's see what our current Content-Encoding is. */
		auto encoding = rctx.getContentEncoding();
		if (!encoding.empty()) {
			ap_remove_output_filter(f);
			return ap_pass_brigade(f->next, bb);
		}

		/* Even if we don't accept this request based on it not having
		 * the Accept-Encoding, we need to note that we were looking
		 * for this header and downstream proxies should be aware of
		 * that.
		 */
		apr_table_mergen(r->headers_out, "Vary", "Accept-Encoding");

		auto reqh = rctx.getRequestHeaders();
		auto resph = rctx.getResponseHeaders();

		auto accepts = reqh.at("Accept-Encoding");
		if (accepts.empty()) {
			ap_remove_output_filter(f);
			return ap_pass_brigade(f->next, bb);
		}

		bool canBeAccepted = false;
		StringView(accepts).split<StringView::Chars<' ', ','>>([&] (StringView &token) {
			if (token == "br") {
				canBeAccepted = true;
			}
		});

		if (canBeAccepted) {
			canBeAccepted = false;
			StringView ctView(resph.at("Content-Type"));
			if (ctView.empty() && r->content_type) {
				ctView = StringView(r->content_type);
			}

			if (!ctView.empty()) {
				auto typeView = ctView.readUntil<StringView::Chars<' ', ';'>>();
				if (typeView.is("text/") || typeView == "application/json" || typeView == "application/javascript" || typeView == "application/cbor") {
					canBeAccepted = true;
				}
			}
		}

		if (!canBeAccepted) {
            ap_remove_output_filter(f);
            return ap_pass_brigade(f->next, bb);
		}

		resph.emplace("Content-Encoding", "br");
		rctx.setContentEncoding("br");

		resph.erase("Content-Length");
		resph.erase("Content-MD5");

		switch (conf->etag_mode) {
		case Server::EtagMode::AddSuffix: {
            auto etag = resph.at("ETag");
            if (!etag.empty()) {
            	StringView etagView(etag); etagView.trimChars<StringView::Chars<'"'>>();
            	resph.emplace("ETag", toString('"', etag, "-br\""));
            }
			break;
		}
		case Server::EtagMode::Remove:
			resph.erase("ETag");
			break;
		case Server::EtagMode::NoChange:
			break;
		}

		/* For 304 responses, we only need to send out the headers. */
		if (r->status == HTTP_NOT_MODIFIED) {
			ap_remove_output_filter(f);
			return ap_pass_brigade(f->next, bb);
		}

		ctx = create_ctx(conf->quality, conf->lgwin, conf->lgblock,
				f->c->bucket_alloc, r->pool);
		f->ctx = ctx;
    }

	while (!APR_BRIGADE_EMPTY(bb)) {
		apr_bucket *e = APR_BRIGADE_FIRST(bb);

		/* Optimization: If we are a HEAD request and bytes_sent is not zero
		 * it means that we have passed the content-length filter once and
		 * have more data to send.  This means that the content-length filter
		 * could not determine our content-length for the response to the
		 * HEAD request anyway (the associated GET request would deliver the
		 * body in chunked encoding) and we can stop compressing.
		 */
		if (r->header_only && r->bytes_sent) {
			ap_remove_output_filter(f);
			return ap_pass_brigade(f->next, bb);
		}

		if (APR_BUCKET_IS_EOS(e)) {
			rv = flush(ctx, BROTLI_OPERATION_FINISH, f);
			if (rv != APR_SUCCESS) {
				return rv;
			}

			/* Leave notes for logging. */
			if (conf->note_input_name) {
				apr_table_setn(r->notes, conf->note_input_name, apr_off_t_toa(r->pool, ctx->total_in));
			}
			if (conf->note_output_name) {
				apr_table_setn(r->notes, conf->note_output_name, apr_off_t_toa(r->pool, ctx->total_out));
			}
			if (conf->note_ratio_name) {
				if (ctx->total_in > 0) {
					int ratio = (int) (ctx->total_out * 100 / ctx->total_in);
					apr_table_setn(r->notes, conf->note_ratio_name, apr_itoa(r->pool, ratio));
				} else {
					apr_table_setn(r->notes, conf->note_ratio_name, "-");
				}
			}

			APR_BUCKET_REMOVE(e);
			APR_BRIGADE_INSERT_TAIL(ctx->bb, e);

			rv = ap_pass_brigade(f->next, ctx->bb);
			apr_brigade_cleanup(ctx->bb);
			apr_pool_cleanup_run(r->pool, ctx, cleanup_ctx);
			return rv;
		} else if (APR_BUCKET_IS_FLUSH(e)) {
			rv = flush(ctx, BROTLI_OPERATION_FLUSH, f);
			if (rv != APR_SUCCESS) {
				return rv;
			}

			APR_BUCKET_REMOVE(e);
			APR_BRIGADE_INSERT_TAIL(ctx->bb, e);

			rv = ap_pass_brigade(f->next, ctx->bb);
			apr_brigade_cleanup(ctx->bb);
			if (rv != APR_SUCCESS) {
				return rv;
			}
		} else if (APR_BUCKET_IS_METADATA(e)) {
			APR_BUCKET_REMOVE(e);
			APR_BRIGADE_INSERT_TAIL(ctx->bb, e);
		} else {
			const char *data;
			apr_size_t len;

			rv = apr_bucket_read(e, &data, &len, APR_BLOCK_READ);
			if (rv != APR_SUCCESS) {
				return rv;
			}
			rv = process_chunk(ctx, data, len, f);
			if (rv != APR_SUCCESS) {
				return rv;
			}
			apr_bucket_delete(e);
		}
	}
	return APR_SUCCESS;
}

NS_SA_END
