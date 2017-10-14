// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "MultiResourceHandler.h"
#include "Resource.h"
#include "AccessControl.h"
#include "Output.h"

NS_SA_BEGIN

MultiResourceHandler::MultiResourceHandler(const Map<String, const Scheme *> &schemes,
		const Transform *t, const AccessControl *a)
: _schemes(schemes), _transform(t), _access(a) { }

bool MultiResourceHandler::isRequestPermitted(Request &rctx) {
	return true;
}

int MultiResourceHandler::onTranslateName(Request &rctx) {
	if (!isRequestPermitted(rctx)) {
		return HTTP_FORBIDDEN;
	}

	auto &data = rctx.getParsedQueryArgs();
	User *user = rctx.getAuthorizedUser();
	if (!user && data.isString("token")) {
		user = rctx.getUser();
	}

	data::Value result;
	for (auto &it : data.asDict()) {
		StringView path(it.first);
		auto scheme = path.readUntil<StringView::Chars<'/'>>();
		if (path.is('/')) {
			++ path;
		}
		auto s_it = _schemes.find(scheme.str());
		if (s_it != _schemes.end()) {
			Resource * resource = Resource::resolve(rctx.storage(), *s_it->second, path.str(), _transform);

			resource->setTransform(_transform);
			resource->setAccessControl(_access);
			resource->setUser(user);
			resource->applyQuery(it.second);
			resource->prepare();

			if (!rctx.isHeaderRequest()) {
				result.setValue(resource->getResultObject(), it.first);
			}
		}
	}

	if (!result.empty()) {
		return writeDataToRequest(rctx, move(result));
	}

	return HTTP_NOT_IMPLEMENTED;
}

int MultiResourceHandler::writeDataToRequest(Request &rctx, data::Value &&result) {
	return output::writeResourceData(rctx, move(result));
}

NS_SA_END
