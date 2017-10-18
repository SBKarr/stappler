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
#include "ResourceHandler.h"
#include "Resource.h"
#include "InputFilter.h"
#include "Session.h"
#include "User.h"
#include "Output.h"
#include "StorageScheme.h"
#include "StorageFile.h"
#include "SPFilesystem.h"

NS_SA_BEGIN

ResourceHandler::ResourceHandler(const storage::Scheme &scheme, const data::TransformMap *t, const AccessControl *a, const data::Value &val)
: _scheme(scheme), _transform(t), _access(a), _value(val) { }

bool ResourceHandler::isRequestPermitted(Request &rctx) {
	return true;
}

int ResourceHandler::onTranslateName(Request &rctx) {
	if (!isRequestPermitted(rctx)) {
		return HTTP_FORBIDDEN;
	}

	if (!rctx.storage()) {
		messages::error("ResourceHandler", "Database connection failed");
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	_method = rctx.getMethod();
	if (_method != Request::Get && _method != Request::Post && _method != Request::Put
			&& _method != Request::Delete && _method != Request::Patch) {
		return HTTP_NOT_IMPLEMENTED;
	}

	auto &data = rctx.getParsedQueryArgs();
	if (data.isString("METHOD")) {
		auto method = data.getString("METHOD");
		if (_method == Request::Get) {
			if (method == "DELETE") {
				_method = Request::Delete;
			}
		} else if (_method == Request::Post) {
			if (method == "PUT") {
				_method = Request::Put;
			} else if (method == "PATCH") {
				_method = Request::Patch;
			}
		}
	}

	_resource = getResource(rctx);
	if (!_resource) {
		return HTTP_NOT_FOUND;
	}

	User *user = rctx.getAuthorizedUser();
	if (!user && data.isString("token")) {
		user = rctx.getUser();
	}

	_resource->setTransform(_transform);
	_resource->setAccessControl(_access);
	_resource->setUser(user);
	_resource->setFilterData(_value);

	if (data.hasValue("pageCount")) {
		_resource->setPageCount(data.getInteger("pageCount"));
	}
	if (data.hasValue("pageFrom")) {
		_resource->setPageFrom(data.getInteger("pageFrom"));
	}

	auto args = rctx.getQueryArgs();
	if (!args.empty() && args.front() == '(') {
		_resource->applyQuery(data);
	} else {
		if (data.hasValue("resolve")) {
			_resource->setResolveOptions(data.getValue("resolve"));
		}
		if (data.hasValue("resolveDepth")) {
			_resource->setResolveDepth(data.getInteger("resolveDepth"));
		}
	}

	_resource->prepare();

	if (_method == Request::Get) {
		if (!rctx.isHeaderRequest()) {
			return writeToRequest(rctx);
		} else {
			return writeInfoToReqest(rctx);
		}
	} else if (_method == Request::Delete) {
		if (_resource->removeObject()) {
			if (data.isString("location")) {
				return rctx.redirectTo(String(data.getString("location")));
			}
			return HTTP_NO_CONTENT;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
	} else if (_method == Request::Post) {
		if (_resource->prepareCreate()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
	} else if (_method == Request::Put) {
		if (_resource->prepareUpdate()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
	} else if (_method == Request::Patch) {
		if (_resource->prepareAppend()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
	}

	return HTTP_NOT_IMPLEMENTED;
}

void ResourceHandler::onInsertFilter(Request &rctx) {
	if (_method == Request::Post || _method == Request::Put || _method == Request::Patch) {
		rctx.setRequiredData(InputConfig::Require::Data | InputConfig::Require::Files);
		rctx.setMaxRequestSize(_resource->getMaxRequestSize());
		rctx.setMaxVarSize(_resource->getMaxVarSize());
		rctx.setMaxFileSize(_resource->getMaxFileSize());

		auto ex = InputFilter::insert(rctx);
		if (ex != InputFilter::Exception::None) {
			if (ex == InputFilter::Exception::TooLarge) {
				rctx.setStatus(HTTP_REQUEST_ENTITY_TOO_LARGE);
			} else if (ex == InputFilter::Exception::Unrecognized) {
				rctx.setStatus(HTTP_UNSUPPORTED_MEDIA_TYPE);
			} else {
				rctx.setStatus(HTTP_BAD_REQUEST);
			}
		}
	} else if (_method != Request::Get && _method != Request::Delete) {
		rctx.setStatus(HTTP_BAD_REQUEST);
		messages::error("Resource", "Input data can not be recieved, no available filters");
	}
}

int ResourceHandler::onHandler(Request &rctx) {
	int num = rctx.getMethod();
	if (num == Request::Get) {
		return DECLINED;
	} else {
		return OK;
	}
}

void ResourceHandler::onFilterComplete(InputFilter *filter) {
	auto rctx = filter->getRequest();
	if (_method == Request::Put) {
		// we should update our resource
		auto result = _resource->updateObject(filter->getData(), filter->getFiles());
		if (result) {
			writeDataToRequest(rctx, move(result));
			rctx.setStatus(HTTP_OK);
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			messages::error("Resource", "Fail to perform update");
		}
	} else if (_method == Request::Post) {
		auto result = _resource->createObject(filter->getData(), filter->getFiles());
		if (result) {
			writeDataToRequest(rctx, move(result));
			rctx.setStatus(HTTP_CREATED);
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			messages::error("Resource", "Fail to perform create");
		}
	} else if (_method == Request::Patch) {
		auto result = _resource->appendObject(filter->getData());
		if (result) {
			writeDataToRequest(rctx, move(result));
			rctx.setStatus(HTTP_OK);
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			messages::error("Resource", "Fail to perform append");
		}
	}
}

Resource *ResourceHandler::getResource(Request &rctx) {
	return Resource::resolve(rctx.storage(), _scheme, _subPath, _value, _transform);
}

int ResourceHandler::writeDataToRequest(Request &rctx, data::Value &&result) {
	return output::writeResourceData(rctx, move(result));
}

int ResourceHandler::writeInfoToReqest(Request &rctx) {
	data::Value result(_resource->getResultObject());
	if (_resource->getType() == ResourceType::File) {
		return output::writeResourceFileHeader(rctx, result);
	}

	return OK;
}

int ResourceHandler::writeToRequest(Request &rctx) {
	data::Value result(_resource->getResultObject());
	if (result) {
		if (_resource->getType() == ResourceType::File) {
			return output::writeResourceFileData(rctx, move(result));
		} else {
			return writeDataToRequest(rctx, move(result));
		}
	}

	return getHintedStatus(HTTP_NOT_FOUND);
}

int ResourceHandler::getHintedStatus(int s) const {
	auto status = _resource->getStatus();
	if (status != HTTP_OK) {
		return status;
	}
	return s;
}

NS_SA_END
