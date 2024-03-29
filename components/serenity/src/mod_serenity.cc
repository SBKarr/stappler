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

#include "Root.h"
#include "Server.h"

#include "ap_provider.h"
#include "mod_log_config.h"
#include "mod_auth.h"

static stappler::serenity::Root s_sharedServer;

extern module AP_MODULE_DECLARE_DATA serenity_module;

using namespace stappler;
using namespace stappler::serenity;

static void *mod_serenity_create_server_config(apr_pool_t *p, server_rec *s) {
	s_sharedServer.setProcPool(s->process->pool);

	return Server(s).getConfig();
}
static void* mod_serenity_merge_config(apr_pool_t* pool, void* BASE, void* ADD) {
	return apr::pool::perform([&] () -> auto {
		return Server::merge(BASE, ADD);
	}, pool, memory::pool::Config);
}
static int mod_serenity_post_config(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s) {
	return apr::pool::perform([&] () -> int {
		return Root::getInstance()->onPostConfig(p, s);
	}, p);
}
static void mod_serenity_child_init(apr_pool_t *p, server_rec *s) {
	Root::getInstance()->onServerChildInit(p, s);
}

static int mod_serenity_check_access_ex(request_rec *r) {
	return Root::getInstance()->onCheckAccess(r);
}
static int mod_serenity_type_checker(request_rec *r) {
	return Root::getInstance()->onTypeChecker(r);
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
    ap_hook_post_config(mod_serenity_post_config, NULL,NULL,APR_HOOK_MIDDLE);
	ap_hook_child_init(mod_serenity_child_init, NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_check_access_ex(mod_serenity_check_access_ex, NULL, NULL, APR_HOOK_FIRST, AP_AUTH_INTERNAL_PER_URI);

    ap_hook_type_checker(mod_serenity_type_checker,NULL,NULL,APR_HOOK_FIRST);

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
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const char *mod_serenity_add_handler_source(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).addHanderSource(apr::string::make_weak(w));
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const char *mod_serenity_add_allow(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).addAllow(apr::string::make_weak(w));
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const char *mod_serenity_set_session_params(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).setSessionParams(apr::string::make_weak(w));
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const char *mod_serenity_set_server_key_params(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).setServerKey(StringView(w));
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const char *mod_serenity_set_root_threads_count(cmd_parms *parms, void *mconfig, const char *w1, const char *w2) {
	apr::pool::perform([&] {
		Root::getInstance()->setThreadsCount(StringView(w1), StringView(w2));
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const char *mod_serenity_set_webhook_params(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).setWebHookParams(apr::string::make_weak(w));
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const char *mod_serenity_set_force_https(cmd_parms *parms, void *mconfig) {
	apr::pool::perform([&] {
		Server(parms->server).setForceHttps();
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const char *mod_serenity_set_protected(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).setProtectedList(apr::string::make_weak(w));
	}, parms->pool, memory::pool::Config);
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

static const char *mod_serenity_set_root_db_params(cmd_parms *parms, void *mconfig, const char *w) {
	if (parms->server->is_virtual) {
		return NULL;
	}

	apr::pool::perform([&] {
		Root::getInstance()->setDbParams(parms->pool, StringView(w));
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const char *mod_serenity_add_create_db(cmd_parms *parms, void *mconfig, const char *arg) {
	if (parms->server->is_virtual) {
		return NULL;
	}

	apr::pool::perform([&] {
		while (*arg) {
			char *name = ap_getword_conf(parms->pool, &arg);
			Root::getInstance()->addDb(parms->pool, StringView(name));
		}
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const char *mod_serenity_set_db_params(cmd_parms *parms, void *mconfig, const char *w) {
	apr::pool::perform([&] {
		Server(parms->server).setDbParams(StringView(w));
	}, parms->pool, memory::pool::Config);
	return NULL;
}

static const command_rec mod_serenity_directives[] = {
	AP_INIT_TAKE1("SerenitySourceRoot", (cmd_func)mod_serenity_set_source_root, NULL, RSRC_CONF,
		"Serenity root dir for source handlers"),
	AP_INIT_RAW_ARGS("SerenitySource", (cmd_func)mod_serenity_add_handler_source, NULL, RSRC_CONF,
		"Serenity handler definition in format (Name:File:Func Args)"),
	AP_INIT_RAW_ARGS("SerenitySession", (cmd_func)mod_serenity_set_session_params, NULL, RSRC_CONF,
		"Serenity session params (name, key, host, maxage, secure)"),
	AP_INIT_TAKE1("SerenityServerKey", (cmd_func)mod_serenity_set_server_key_params, NULL, RSRC_CONF,
		"Security key for serenity server, that will be used for cryptographic proposes"),
	AP_INIT_RAW_ARGS("SerenityWebHook", (cmd_func)mod_serenity_set_webhook_params, NULL, RSRC_CONF,
		"Serenity webhook error reporter address in format: SerenityWebHook name=<name> url=<url>"),
	AP_INIT_NO_ARGS("SerenityForceHttps", (cmd_func)mod_serenity_set_force_https, NULL, RSRC_CONF,
		"Host should forward insecure requests to secure connection"),
	AP_INIT_RAW_ARGS("SerenityProtected", (cmd_func)mod_serenity_set_protected, NULL, RSRC_CONF,
		"Space-separated list of location prefixes, which should be invisible for clients"),
	AP_INIT_RAW_ARGS("SerenityServerNames", (cmd_func)mod_serenity_set_server_names, NULL, RSRC_CONF,
		"Space-separated list of server names (first would be ServerName, others - ServerAliases)"),
	AP_INIT_RAW_ARGS("SerenityAllowIp", (cmd_func)mod_serenity_add_allow, NULL, RSRC_CONF,
		"Additional IPv4 masks to thrust when admin access is requested"),
	AP_INIT_TAKE2("SerenityRootThreadsCount", (cmd_func)mod_serenity_set_root_threads_count, NULL, RSRC_CONF,
		"<init> <max> - size of root thread pool for async tasks"),

	AP_INIT_RAW_ARGS("SerenityRootDbParams", (cmd_func)mod_serenity_set_root_db_params, NULL, RSRC_CONF,
		"Serenity database parameters for root connections (driver, host, dbname, user, password, other driver-defined params), has no effect in vhost"),
	AP_INIT_RAW_ARGS("SerenityRootCreateDb", (cmd_func)mod_serenity_add_create_db, NULL, RSRC_CONF,
		"Space-separated list of databases, that need to be created"),
	AP_INIT_RAW_ARGS("SerenityDbParams", (cmd_func)mod_serenity_set_db_params, NULL, RSRC_CONF,
		"Enable custom dbd connections for server with parameters (driver, host, dbname, user, password, other driver-defined params). "
		"Driver and parameters, that was not defined, inherited from SerenityRootDbParams. "
		"Parameter dbname will be automatically added to CreateDb list"),

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
