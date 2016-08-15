/*
 * DynamicHandler.h
 *
 *  Created on: 22 февр. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_SERVER_SERVERCOMPONENT_H_
#define SERENITY_SRC_SERVER_SERVERCOMPONENT_H_

#include "Server.h"
#include "AccessControl.h"

NS_SA_BEGIN

class ServerComponent : public AllocPool {
public:
	using Symbol = ServerComponent * (*) (Server &serv, const String &name, const data::Value &dict);
	using Scheme = storage::Scheme;

	ServerComponent(Server &serv, const String &name, const data::Value &dict);
	virtual ~ServerComponent() { }

	virtual void onChildInit(Server &);

	const data::Value & getConfig() const { return _config; }
	const String & getName() const { return _name; }

	storage::Scheme * exportScheme(storage::Scheme *);

protected:
	Server _server;
	String _name;
	data::Value _config;
};


NS_SA_END

#endif /* SERENITY_SRC_SERVER_SERVERCOMPONENT_H_ */
