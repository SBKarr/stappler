// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
#include "StorageScheme.h"
#include "StorageFile.h"
#include "SPFilesystem.h"

NS_SA_BEGIN

ResourceHandler::ResourceHandler(storage::Scheme *scheme, const data::TransformMap *t, AccessControl *a, const data::Value &val)
: _scheme(scheme), _transform(t), _access(a), _value(val) { }

bool ResourceHandler::isRequestPermitted(Request &rctx) {
	return true;
}

int ResourceHandler::onTranslateName(Request &rctx) {
	if (!isRequestPermitted(rctx)) {
		return HTTP_FORBIDDEN;
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

	User *user = nullptr;
	if (data.isString("token")) {
		user = rctx.getUser();
	}

	_resource->setTransform(_transform);
	_resource->setAccessControl(_access);
	_resource->setUser(user);
	_resource->setFilterData(_value);

	if (data.hasValue("resolve")) {
		_resource->setResolveOptions(data.getValue("resolve"));
	}
	if (data.hasValue("resolveDepth")) {
		_resource->setResolveDepth(data.getInteger("resolveDepth"));
	}
	if (data.hasValue("pageFrom") || data.hasValue("pageCount")) {
		auto from = data.getInteger("pageFrom");
		auto count = data.getInteger("pageCount", maxOf<size_t>());
		_resource->setPagination(from, count);
	}

	if (_method == M_GET) {
		if (!rctx.isHeaderRequest()) {
			return writeToRequest(rctx);
		} else {
			return writeInfoToReqest(rctx);
		}
	} else if (_method == M_DELETE) {
		if (_resource->removeObject()) {
			if (data.isString("location")) {
				return rctx.redirectTo(String(data.getString("location")));
			}
			return HTTP_NO_CONTENT;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
	} else if (_method == M_POST) {
		if (_resource->prepareCreate()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
	} else if (_method == M_PUT) {
		if (_resource->prepareUpdate()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
	} else if (_method == M_PATCH) {
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
	if (_method == M_PUT) {
			// we should update our resource
		auto result = _resource->updateObject(filter->getData(), filter->getFiles());
		if (result) {
			writeDataToRequest(rctx, result);
			rctx.setStatus(HTTP_OK);
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			messages::error("Resource", "Fail to perform update");
		}
	} else if (_method == M_POST) {
		auto result = _resource->createObject(filter->getData(), filter->getFiles());
		if (result) {
			writeDataToRequest(rctx, result);
			rctx.setStatus(HTTP_CREATED);
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			messages::error("Resource", "Fail to perform create");
		}
	}
}

Resource *ResourceHandler::getResource(Request &rctx) {
	return Resource::resolve(rctx.storage(), _scheme, _subPath, _value, _transform);
}

void ResourceHandler::setFileParams(Request &rctx, const data::Value &file) {
	auto h = rctx.getResponseHeaders();
	h.emplace("X-FileModificationTime", apr_psprintf(h.get_allocator(), "%ld", file.getInteger("mtime")));
	h.emplace("X-FileSize", apr_psprintf(h.get_allocator(), "%ld", file.getInteger("size")));
	h.emplace("X-FileId", apr_psprintf(h.get_allocator(), "%ld",  file.getInteger("__oid")));
	if (file.isString("location")) {
		h.emplace("X-FileLocation", apr_psprintf(h.get_allocator(), "%s", file.getString("location").c_str()));
	}
	h.emplace("Content-Type", file.getString("type"));
	rctx.setContentType(String(file.getString("type")));
}

void ResourceHandler::performApiObject(Request &rctx, storage::Scheme *scheme, data::Value &obj) {
	auto id = obj.getInteger("__oid");
	auto path = rctx.server().getResourcePath(_scheme);
	if (!path.empty() && id > 0) {
		data::Value actions;
		apr::ostringstream remove;
		remove << path << "id" << id << '|' << path << "id" << id << "?METHOD=DELETE"
				<< "&location=" << string::urlencode(rctx.getUnparsedUri());

		String token;
		if (auto session = rctx.getSession()) {
			if (auto user = session->getUser()) {
				if (user->isAdmin()) {
					auto & stoken = session->getSessionToken();
					token = string::urlencode(base64::encode(stoken.data(), stoken.size()));
					remove << "&token=" << token;

				}
			}
		}
		actions.setString(remove.str(), "remove");
		obj.setValue(std::move(actions), "~ACTIONS~");

		for (auto &it : obj.asDict()) {
			auto f = scheme->getField(it.first);
			if (f && f->isFile() && it.second.isInteger()) {
				if (!token.empty()) {
					it.second.setString(toString("~~", f->getName(), "|", path, "id", id, "/", it.first, "?token=", token));
				} else {
					it.second.setString(toString("~~", f->getName(), "|", path, "id", id, "/", it.first));
				}
			}
		}
	}
}

void ResourceHandler::performApiFilter(Request &rctx, storage::Scheme *scheme, data::Value &val) {
	if (val.isDictionary()) {
		performApiObject(rctx, scheme, val);
	} else if (val.isArray()) {
		for (auto &it : val.asArray()) {
			performApiObject(rctx, scheme, it);
		}
	}
}

int ResourceHandler::writeDataToRequest(Request &rctx, data::Value &result) {
	auto &vars = rctx.getParsedQueryArgs();
	data::Value data;

	data.setInteger(apr_time_now(), "date");
#if DEBUG
	auto &debug = rctx.getDebugMessages();
	if (!debug.empty()) {
		data.setArray(debug, "debug");
	}
#endif
	auto &error = rctx.getErrorMessages();
	if (!error.empty()) {
		data.setArray(error, "errors");
	}

	if (vars.getString("pretty") == "api") {
		performApiFilter(rctx, _resource->getScheme(), result);
	}

	data.setValue(std::move(result), "result");
	data.setBool(true, "OK");
	rctx.writeData(data, true);

	return DONE;
}

int ResourceHandler::writeInfoToReqest(Request &rctx) {
	data::Value result(_resource->getResultObject());
	if (_resource->getScheme() == rctx.server().getFileScheme()) {
		data::Value file(result.isArray()?std::move(result.getValue(0)):std::move(result));

		if (!file) {
			return HTTP_NOT_FOUND;
		}

		setFileParams(rctx, file);
		return DONE;
	}

	return OK;
}
int ResourceHandler::writeToRequest(Request &rctx) {
	data::Value result(_resource->getResultObject());
	if (result) {
		if (_resource->getScheme() == rctx.server().getFileScheme()) {
			data::Value file(result.isArray()?std::move(result.getValue(0)):std::move(result));
			auto path = storage::File::getFilesystemPath((uint64_t)file.getInteger("__oid"));

			auto &queryData = rctx.getParsedQueryArgs();
			if (queryData.getBool("stat")) {
				file.setBool(filesystem::exists(path), "exists");
				return writeDataToRequest(rctx, file);
			}

			auto &loc = file.getString("location");
			if (filesystem::exists(path) && loc.empty()) {
				rctx.setContentType(String(file.getString("type")));
				rctx.setFilename(std::move(path));
				return OK;
			}

			if (!loc.empty()) {
				rctx.setFilename(nullptr);
				return rctx.redirectTo(std::move(loc));
			}

			return HTTP_NOT_FOUND;
		} else {
			return writeDataToRequest(rctx, result);
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
