/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
	virtual int onTranslateName(Request &) override;
	virtual bool processDataHandler(Request &, data::Value &result, data::Value &input) override;
};

/* WebSocket shell interface */
class ShellSocket : public websocket::Manager {
public:
	ShellSocket(Server serv) : Manager(serv) { }

	virtual Handler * onAccept(const Request &, mem::pool_t *) override;
	virtual bool onBroadcast(const data::Value &) override;
};

/* WebSocket shell interface GUI */
class ShellGui : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual int onPostReadRequest(Request &) override;

	virtual void onInsertFilter(Request &) override;
	virtual int onHandler(Request &) override;

	virtual void onFilterComplete(InputFilter *f) override;

protected:
	Resource *_resource = nullptr;
};

/* WebSocket shell interface GUI */
class ServerGui : public DataHandler {
public:
	static void defineBasics(pug::Context &exec, Request &req, User *u);

	ServerGui() { _allow = AllowMethod::Get | AllowMethod::Post; _maxRequestSize = 512; }

	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual int onTranslateName(Request &) override;
	virtual void onFilterComplete(InputFilter *f) override;
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
	bool processImageTest(Request &rctx, data::Value &ret, const data::Value &input, db::InputFile &);
};

class ErrorsGui : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual int onTranslateName(Request &) override;
};

class HandlersGui : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual int onTranslateName(Request &) override;
};

class ReportsGui : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual int onTranslateName(Request &) override;
};

class VirtualGui : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual int onTranslateName(Request &) override;
	virtual int onHandler(Request &) override;

	virtual void onInsertFilter(Request &) override;
	virtual void onFilterComplete(InputFilter *filter) override;

protected:
	data::Value readMeta(StringView) const;
	void writeData(data::Value &) const;

	bool createArticle(const data::Value &);
	bool createCategory(const data::Value &);

	void makeMdContents(Request &req, pug::Context &exec, StringView path) const;
	data::Value makeDirInfo(StringView path, bool forFile = false) const;

	bool _virtual = true;
#if DEBUG
	bool _editable = true;
#else
	bool _editable = false;
#endif
};

class VirtualFilesystem : public RequestHandler {
public:
	virtual bool isRequestPermitted(Request &) override { return true; }
	virtual int onTranslateName(Request &) override;
};

void registerTools(const String &prefix, Server &serv);

struct VirtualFile {
	static VirtualFile add(const StringView &, const StringView &);
	static StringView get(const StringView &);

	static const VirtualFile *getList(size_t &);

	StringView name;
	StringView content;
};

const char * getVersionString();
const char * getDocumentsPath();
const char * getCompileDate();
Time getCompileUnixTime();

NS_SA_EXT_END(tools)

#endif /* SERENITY_SRC_TOOLS_TOOLS_H_ */
