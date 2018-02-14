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

#include "Define.h"
#include "MultiResourceHandler.h"
#include "Resource.h"
#include "AccessControl.h"
#include "Output.h"
#include "StorageScheme.h"

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

	if (!rctx.storage()) {
		messages::error("ResourceHandler", "Database connection failed");
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	auto &data = rctx.getParsedQueryArgs();
	User *user = rctx.getAuthorizedUser();
	if (!user && data.isString("token")) {
		user = rctx.getUser();
	}

	int64_t targetDelta = 0;
	Time deltaMax;
	data::Value result;
	data::Value delta;
	Vector<Pair<String, Resource *>> resources;
	resources.reserve(data.size());

	if (data.isInteger("delta")) {
		targetDelta = data.getInteger("delta");
	}

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
			if (targetDelta > 0 && resource->isDeltaApplicable() && !resource->getQueryDelta()) {
				resource->setQueryDelta(Time::microseconds(targetDelta));
			}
			resource->prepare();

			if (resource->hasDelta()) {
				deltaMax = max(deltaMax, resource->getSourceDelta());
				delta.setInteger(resource->getSourceDelta().toMicroseconds(), it.first);
			}

			resources.emplace_back(it.first, resource);
		}
	}

	if (deltaMax && delta.size() == resources.size()) {
		rctx.getResponseHeaders().emplace("Last-Modified", deltaMax.toHttp());
		if (targetDelta > 0) {
			const String modified = rctx.getRequestHeaders().at("if-modified-since");
			if (Time::fromHttp(modified).toSeconds() >= deltaMax.toSeconds()) {
				return HTTP_NOT_MODIFIED;
			}
		}
	}

	for (auto &it : resources) {
		if (!rctx.isHeaderRequest()) {
			result.setValue(it.second->getResultObject(), it.first);
		}
	}

	if (!result.empty()) {
		resultData.setValue(move(delta), "delta");
		return writeDataToRequest(rctx, move(result));
	}

	return HTTP_NOT_IMPLEMENTED;
}

int MultiResourceHandler::writeDataToRequest(Request &rctx, data::Value &&result) {
	return output::writeResourceData(rctx, move(result), move(resultData));
}

NS_SA_END
