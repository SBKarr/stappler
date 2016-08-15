/*
 * SPSocketServer.h
 *
 *  Created on: 11 янв. 2016 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_CORE_STRUCT_SPSOCKETSERVER_H_
#define STAPPLER_SRC_CORE_STRUCT_SPSOCKETSERVER_H_

#include "SPDefine.h"

NS_SP_BEGIN

class SocketServer {
public:
	bool listen(uint16_t port = 0);
	void cancel();

	bool isListening() const;

	void log(const char *str, size_t n);

protected:
	class ServerThread;
	ServerThread *_thread = nullptr;
};

NS_SP_END

#endif /* STAPPLER_SRC_CORE_STRUCT_SPSOCKETSERVER_H_ */
