/*
   Copyright 2013 Roman "SBKarr" Katuntsev, LLC St.Appler

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "server/Root.h"
#include "server/Server.h"

#include "mod_log_config.h"

static stappler::serenity::Root s_sharedServer;

extern module AP_MODULE_DECLARE_DATA serenity_module;

USING_NS_SA;

static void *mod_serenity_create_server_config(apr_pool_t *p, server_rec *s) {
	s_sharedServer.setProcPool(s->process->pool);

	return Server(s).getConfig();
}
static void* mod_serenity_merge_config(apr_pool_t* pool, void* BASE, void* ADD) {
	return AllocStack::perform([&] () -> auto {
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

static void mod_serenity_register_hooks(apr_pool_t *pool) {
	ap_hook_child_init(mod_serenity_child_init, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_open_logs(mod_serenity_open_logs,NULL,NULL,APR_HOOK_FIRST);

	ap_hook_post_read_request(mod_serenity_post_read_request, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_translate_name(mod_serenity_translate_name, NULL, NULL, APR_HOOK_LAST);
	ap_hook_quick_handler(mod_serenity_quick_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_insert_filter(mod_serenity_insert_filter, NULL, NULL, APR_HOOK_LAST);
    ap_hook_handler(mod_serenity_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

static const char *mod_serenity_set_source_root(cmd_parms *parms, void *mconfig, const char *w) {
	AllocStack::perform([&] {
		Server(parms->server).setSourceRoot(apr::string::make_weak(w));
	}, parms->pool);
	return NULL;
}

static const char *mod_serenity_add_handler_source(cmd_parms *parms, void *mconfig, const char *w) {
	AllocStack::perform([&] {
		Server(parms->server).addHanderSource(apr::string::make_weak(w));
	}, parms->pool);
	return NULL;
}

static const char *mod_serenity_set_session_params(cmd_parms *parms, void *mconfig, const char *w) {
	AllocStack::perform([&] {
		Server(parms->server).setSessionParams(apr::string::make_weak(w));
	}, parms->pool);
	return NULL;
}

static const command_rec mod_serenity_directives[] = {
	AP_INIT_TAKE1("SerenitySourceRoot", (cmd_func)mod_serenity_set_source_root, NULL, RSRC_CONF,
		"Serenity root dir for source handlers"),
	AP_INIT_RAW_ARGS("SerenitySource", (cmd_func)mod_serenity_add_handler_source, NULL, RSRC_CONF,
		"Serenity handler definition in format (Name File Func Args)"),
	AP_INIT_RAW_ARGS("SerenitySession", (cmd_func)mod_serenity_set_session_params, NULL, RSRC_CONF,
		"Serenity session params (name, key, host, maxage, secure)"),
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
