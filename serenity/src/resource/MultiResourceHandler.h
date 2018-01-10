/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_RESOURCE_MULTIRESOURCEHANDLER_H_
#define SERENITY_SRC_RESOURCE_MULTIRESOURCEHANDLER_H_

#include "RequestHandler.h"

NS_SA_BEGIN

class MultiResourceHandler : public RequestHandler {
public:
	using Scheme = storage::Scheme;
	using Transform = data::TransformMap;

	MultiResourceHandler(const Map<String, const Scheme *> &, const Transform * = nullptr, const AccessControl * = nullptr);

	virtual bool isRequestPermitted(Request &) override;
	virtual int onTranslateName(Request &) override;

protected:
	int writeDataToRequest(Request &rctx, data::Value &&objs);

	data::Value resultData;

	Map<String, const Scheme *> _schemes;
	const Transform * _transform = nullptr;
	const AccessControl *_access = nullptr;
};

NS_SA_END

#endif /* SERENITY_SRC_RESOURCE_MULTIRESOURCEHANDLER_H_ */
