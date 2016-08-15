/*
 * Tools.h
 *
 *  Created on: 21 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_TOOLS_TOOLS_H_
#define SERENITY_SRC_TOOLS_TOOLS_H_

#include "WebSocket.h"
#include "RequestHandler.h"

NS_SA_EXT_BEGIN(tools)

/* Simple auth handler, creates ащгк virtual pages:
 *
 * $PREFIX$/auth/setup? name=$NAME$ & passwd=$PASSWD$
 *  - create new admin user, if there is no other users
 *
 * $PREFIX$/auth/login? name=$NAME$ & passwd=$PASSWD$ & maxAge=$MAXAGE$
 *  - init new session for user, creates new token pair for $MAXAGE$ (720 seconds max)
 *
 * $PREFIX$/auth/update? token=$TOKEN$ & maxAge=$MAXAGE$
 *  - updates session token pair for $MAXAGE$ (720 seconds max)
 *
 * $PREFIX$/auth/cancel? token=$TOKEN$
 *  - cancel session
 *
 */
class AuthHandler : public DataHandler {
public:
	virtual bool isRequestPermitted(Request &) override;
	virtual bool processDataHandler(Request &, data::Value &result, data::Value &input) override;
};

/* WebSocket shell interface */
class ShellSocket : public websocket::Manager {
public:
	virtual Handler * onAccept(const Request &) override;
	virtual bool onBroadcast(const data::Value &) override;
};

/* WebSocket shell interface GUI */
class ShellGui : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual int onPostReadRequest(Request &) override;
};

class TestHandler : public DataHandler {
public:
	TestHandler();
	virtual bool isRequestPermitted(Request &) override;
	virtual bool processDataHandler(Request &, data::Value &, data::Value &) override;

protected:
	bool processEmailTest(Request &rctx, data::Value &ret, const data::Value &input);
	bool processUrlTest(Request &rctx, data::Value &ret, const data::Value &input);
	bool processUserTest(Request &rctx, data::Value &ret, const data::Value &input);
	bool processImageTest(Request &rctx, data::Value &ret, const data::Value &input, InputFile &);
};

/* WebSocket shell interface GUI */
class VirtualFilesystem : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual int onPostReadRequest(Request &) override;
};

void registerTools(const String &prefix, Server &serv);

struct VirtualFile {
	VirtualFile(const char *, const char *);

	const char *name;
	const char *content;
};

const char * getCompileDate();

NS_SA_EXT_END(tools)

#endif /* SERENITY_SRC_TOOLS_TOOLS_H_ */
