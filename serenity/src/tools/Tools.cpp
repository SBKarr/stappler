/*
 * Tools.cpp
 *
 *  Created on: 21 апр. 2016 г.
 *      Author: sbkarr
 */

#include "Define.h"
#include "Tools.h"

NS_SA_EXT_BEGIN(tools)

void registerTools(const String &prefix, Server &serv) {
	serv.addHandler(prefix + config::getServerToolsShell(), SA_HANDLER(tools::ShellGui));
	serv.addWebsocket(prefix + config::getServerToolsShell(), new tools::ShellSocket());

	serv.addHandler(prefix + config::getServerToolsAuth(), SA_HANDLER(tools::AuthHandler));
	serv.addHandler(prefix + config::getServerVirtualFilesystem(), SA_HANDLER(tools::VirtualFilesystem));
}

NS_SA_EXT_END(tools)
