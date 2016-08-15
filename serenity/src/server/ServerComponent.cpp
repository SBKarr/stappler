/*
 * DynamicHandler.cpp
 *
 *  Created on: 22 февр. 2016 г.
 *      Author: sbkarr
 */

#include "Define.h"
#include "ServerComponent.h"

NS_SA_BEGIN

ServerComponent::ServerComponent(Server &serv, const String &name, const data::Value &dict)
: _server(serv), _name(name), _config(dict) { }

void ServerComponent::onChildInit(Server &serv) { }

storage::Scheme * ServerComponent::exportScheme(storage::Scheme *scheme) {
	return _server.exportScheme(scheme);
}

NS_SA_END
