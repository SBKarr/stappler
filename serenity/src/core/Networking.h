/*
 * Networking.h
 *
 *  Created on: 12 мая 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_CORE_NETWORKING_H_
#define SERENITY_SRC_CORE_NETWORKING_H_

#include "Define.h"
#include "SPNetworkHandle.h"

NS_SA_EXT_BEGIN(network)

class Handle : public NetworkHandle {
public:
	Handle(Method method, const String &url);

	data::Value performDataQuery();
	data::Value performDataQuery(const data::Value &, data::EncodeFormat = data::EncodeFormat::DefaultFormat);
	bool performCallbackQuery(const IOCallback &);
	bool performProxyQuery(Request &);
};

class Mail : public NetworkHandle {
public:
	Mail(const String &url, const String &user, const String &passwd);

	bool send(apr::ostringstream &stream);
};

NS_SA_EXT_END(network)

#endif /* SERENITY_SRC_CORE_NETWORKING_H_ */
