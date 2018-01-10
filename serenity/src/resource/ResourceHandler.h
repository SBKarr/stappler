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

#ifndef SERENITY_SRC_RESOURCE_RESOURCEHANDLER_H_
#define SERENITY_SRC_RESOURCE_RESOURCEHANDLER_H_

#include "RequestHandler.h"

NS_SA_BEGIN

class ResourceHandler : public RequestHandler {
public:
	using Scheme = storage::Scheme;
	using Transform = data::TransformMap;

	ResourceHandler(const Scheme &scheme, const Transform * = nullptr,
			const AccessControl * = nullptr, const data::Value & = data::Value());

	virtual bool isRequestPermitted(Request &) override;

	virtual int onTranslateName(Request &) override;
	virtual void onInsertFilter(Request &) override;
	virtual int onHandler(Request &) override;

	virtual void onFilterComplete(InputFilter *f) override;

public: // request interface
	int writeInfoToReqest(Request &rctx);
	int writeToRequest(Request &rctx);

protected:
	int writeDataToRequest(Request &rctx, data::Value &&objs);
	int getHintedStatus(int) const;

	virtual Resource *getResource(Request &);

	Request::Method _method = Request::Get;
	const storage::Scheme &_scheme;
	const data::TransformMap * _transform = nullptr;
	const AccessControl *_access = nullptr;
	Resource *_resource = nullptr;
	data::Value _value;
};

NS_SA_END

#endif /* SERENITY_SRC_RESOURCE_RESOURCEHANDLER_H_ */
