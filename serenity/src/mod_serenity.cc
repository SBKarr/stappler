/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "server/Root.h"
#include "server/Server.h"

#include "ap_provider.h"
#include "mod_log_config.h"
#include "mod_auth.h"

static stappler::serenity::Root s_sharedServer;

extern module AP_MODULE_DECLARE_DATA serenity_module;

USING_NS_SP;
USING_NS_SA;

static void *mod_serenity_create_server_config(apr_pool_t *p, server_rec *s) {
	s_sharedServer.setProcPool(s->process->pool);

	return Server(s).getConfig();
}
static void* mod_serenity_merge_config(apr_pool_t* pool, void* BASE, void* ADD) {
	return apr::pool::perform([&] () -> auto {
		return Server::merge(BASE, ADD);
	}, pool);
}
static void mod_serenity_child_init(apr_pool_t *p, server_rec *s) {
	Root::getInstance()->onServerChildInit(p, s);
}
static int mod_serenity_open_logs(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s) {
	Root::getInstance()->onOpenLogs(pconf, plog, ptemp, s);
	return OK;
}

static int mod_serenity_check_access_ex(request_rec *r) {
	return Root::getInstance()->onCheckAccess(r);
}
static int mod_serenity_post_read_request(request_rec *r) {
	return Root::getInstance()->onPostReadRequest(r);
}
static int mod_serenity_translate_name(request_rec *r) {
	return Root::getInstance()->onTranslateName(r);
}
static int mod_serenity_quick_handler(request_rec *r, int v) {
	return Root::getInstance()->onQuickHandler(r, v);
}
static void mod_serenity_insert_filter(request_rec *r) {
	Root::getInstance()->onInsertFilter(r);
}
static int mod_serenity_handler(request_rec *r) {
	return Root::getInstance()->onHandler(r);
}

static apr_status_t mod_serenity_compress(ap_filter_t *f, apr_bucket_brigade *bb) {
	return apr::pool::perform([&] () -> int {
		return compress_filter(f, bb);
	}, f->r);
}

static void mod_serenity_register_hooks(apr_pool_t *pool) {
	ap_hook_child_init(mod_serenity_child_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_open_logs(mod_serenity_open_logs, NULL, NULL, APR_HOOK_FIRST);

    ap_hook_check_access_ex(mod_serenity_check_access_ex, NULL, NULL, APR_HOOK_FIRST, AP_AUTH_INTERNAL_PER_URI);

	ap_hook_post_read_request(mod_serenity_post_read_request, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_translate_name(mod_serenity_translate_name, NULL, NULL, APR_HOOK_LAST);
	ap_hook_quick_handler(mod_serenity_quick_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_insert_filter(mod_serenity_insert_filter, NULL, NULL, APR_HOOK_LAST);
    ap_hook_handler(mod_serenity_handler, NULL, NULL, APR_HOOK_MIDDLE);

    ap_register_output_filter("Serenity::Compress", mod_serenity_compress, NULL, AP_FTYPE_CONTENT_SET);
}

static const char *mod_serenity_set_source_root(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).setSourceRoot(apr::string::make_weak(w));
	}, parms->pool);
	return NULL;
}

static const char *mod_serenity_add_handler_source(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).addHanderSource(apr::string::make_weak(w));
	}, parms->pool);
	return NULL;
}

static const char *mod_serenity_set_session_params(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).setSessionParams(apr::string::make_weak(w));
	}, parms->pool);
	return NULL;
}

static const char *mod_serenity_set_webhook_params(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).setWebHookParams(apr::string::make_weak(w));
	}, parms->pool);
	return NULL;
}

static const char *mod_serenity_set_force_https(cmd_parms *parms, void *mconfig) {
	apr::pool::perform([&] {
		Server(parms->server).setForceHttps();
	}, parms->pool);
	return NULL;
}

static const char *mod_serenity_set_protected(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).setProtectedList(apr::string::make_weak(w));
	}, parms->pool);
	return NULL;
}

static const char *mod_serenity_set_server_names(cmd_parms *parms, void *mconfig, const char *arg) {
    if (!parms->server->names) {
        return "Only used in <VirtualHost>";
    }

    bool hostname = false;
	while (*arg) {
		char **item, *name = ap_getword_conf(parms->pool, &arg);
		if (!hostname) {
			parms->server->server_hostname = apr_pstrdup(parms->pool, name);
			hostname = true;
		} else {
			if (ap_is_matchexp(name)) {
				item = (char **) apr_array_push(parms->server->wild_names);
			} else {
				item = (char **) apr_array_push(parms->server->names);
			}
			*item = name;
		}
	}

    return NULL;
}

static const command_rec mod_serenity_directives[] = {
	AP_INIT_TAKE1("SerenitySourceRoot", (cmd_func)mod_serenity_set_source_root, NULL, RSRC_CONF,
		"Serenity root dir for source handlers"),
	AP_INIT_RAW_ARGS("SerenitySource", (cmd_func)mod_serenity_add_handler_source, NULL, RSRC_CONF,
		"Serenity handler definition in format (Name:File:Func Args)"),
	AP_INIT_RAW_ARGS("SerenitySession", (cmd_func)mod_serenity_set_session_params, NULL, RSRC_CONF,
		"Serenity session params (name, key, host, maxage, secure)"),
	AP_INIT_RAW_ARGS("SerenityWebHook", (cmd_func)mod_serenity_set_webhook_params, NULL, RSRC_CONF,
		"Serenity webhook error reporter address in format: SerenityWebHook name=<name> url=<url>"),
	AP_INIT_NO_ARGS("SerenityForceHttps", (cmd_func)mod_serenity_set_force_https, NULL, RSRC_CONF,
		"Host should forward requests to secure connection"),
	AP_INIT_RAW_ARGS("SerenityProtected", (cmd_func)mod_serenity_set_protected, NULL, RSRC_CONF,
		"Space-separated list of location prefixes, which should be invisible for clients"),
	AP_INIT_RAW_ARGS("SerenityServerNames", (cmd_func)mod_serenity_set_server_names, NULL, RSRC_CONF,
		"Space-separated list of server names (first would be ServerName, others - ServerAliases)"),
    { NULL }
};

module AP_MODULE_DECLARE_DATA serenity_module = {
    STANDARD20_MODULE_STUFF,
    NULL, /* Per-directory configuration handler */
    NULL,  /* Merge handler for per-directory configurations */
    mod_serenity_create_server_config, /* Per-server configuration handler */
    mod_serenity_merge_config,  /* Merge handler for per-server configurations */
    mod_serenity_directives, 	/* Any directives we may have for httpd */
    mod_serenity_register_hooks  /* Our hook registering function */
};
