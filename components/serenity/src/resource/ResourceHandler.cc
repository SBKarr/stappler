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
#include "ResourceHandler.h"
#include "ResourceTemplates.h"

#include "SPFilesystem.h"
#include "Resource.h"
#include "InputFilter.h"
#include "Session.h"
#include "Output.h"

NS_SA_BEGIN

ResourceHandler::ResourceHandler(const storage::Scheme &scheme, const data::Value &val)
: _scheme(scheme), _value(val) { }

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

	switch (_method) {
	case Request::Get: {
		const String modified = rctx.getRequestHeaders().at("if-modified-since");
		auto mt = modified.empty() ? 0 : Time::fromHttp(modified).toSeconds();

		if (auto d = _resource->getSourceDelta()) {
			rctx.getResponseHeaders().emplace("Last-Modified", d.toHttp());
			if (mt >= d.toSeconds()) {
				return HTTP_NOT_MODIFIED;
			}
		}

		if (mt > 0 && _resource->getType() == ResourceType::Object) {
			if (auto res = dynamic_cast<ResourceObject *>(_resource)) {
				if (auto objMtime = res->getObjectMtime()) {
					if (mt >= uint64_t(objMtime / 1000000)) {
						return HTTP_NOT_MODIFIED;
					}
				}
			}
		}

		_resource->prepare(storage::QueryList::SimpleGet);

		if (!rctx.isHeaderRequest()) {
			return writeToRequest(rctx);
		} else {
			return writeInfoToReqest(rctx);
		}
		break;
	}
	case Request::Delete:
		_resource->prepare();
		if (_resource->removeObject()) {
			if (data.isString("location")) {
				return rctx.redirectTo(String(data.getString("location")));
			} else if (data.isString("target")) {
				auto &target = data.getString("target");
				if (!target.empty() && (StringView(target).starts_with(StringView(rctx.getFullHostname())) || target[0] == '/')) {
					return rctx.redirectTo(String(target));
				}
			}
			return HTTP_NO_CONTENT;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
		break;
	case Request::Post:
		_resource->prepare();
		if (_resource->prepareCreate()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
		break;
	case Request::Put:
		_resource->prepare();
		if (_resource->prepareUpdate()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
		break;
	case Request::Patch:
		_resource->prepare();
		if (_resource->prepareAppend()) {
			return DECLINED;
		} else {
			return getHintedStatus(HTTP_FORBIDDEN);
		}
		break;
	default:
		break;
	}

	return HTTP_NOT_IMPLEMENTED;
}

void ResourceHandler::onInsertFilter(Request &rctx) {
	if (_method == Request::Post || _method == Request::Put || _method == Request::Patch) {
		rctx.setRequiredData(db::InputConfig::Require::Data | db::InputConfig::Require::Files);
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
			auto &target = rctx.getParsedQueryArgs().getValue("target");
			if (target.isString() && (StringView(target.getString()).starts_with(StringView(rctx.getFullHostname())) || target.getString()[0] == '/')) {
				rctx.setStatus(rctx.redirectTo(String(target.getString())));
			} else {
				writeDataToRequest(rctx, move(result));
				rctx.setStatus(HTTP_OK);
			}
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			if (result.isNull()) {
				messages::error("Resource", "Fail to perform update", data::Value(filter->getData()));
			}
		}
	} else if (_method == Request::Post) {
		auto &d = filter->getData();
		auto tmp = d;
		auto result = _resource->createObject(d, filter->getFiles());
		if (result) {
			auto &target = rctx.getParsedQueryArgs().getValue("target");
			if (target.isString() && (StringView(target.getString()).starts_with(StringView(rctx.getFullHostname())) || target.getString()[0] == '/')) {
				rctx.setStatus(rctx.redirectTo(String(target.getString())));
			} else {
				writeDataToRequest(rctx, move(result));
				rctx.setStatus(HTTP_CREATED);
			}
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			if (result.isNull()) {
				messages::error("Resource", "Fail to perform create", data::Value(move(tmp)));
			}
		}
	} else if (_method == Request::Patch) {
		auto result = _resource->appendObject(filter->getData());
		if (result) {
			auto &target = rctx.getParsedQueryArgs().getValue("target");
			if (target.isString() && (StringView(target.getString()).starts_with(StringView(rctx.getFullHostname())) || target.getString()[0] == '/')) {
				rctx.setStatus(rctx.redirectTo(String(target.getString())));
			} else {
				writeDataToRequest(rctx, move(result));
				rctx.setStatus(HTTP_OK);
			}
		} else {
			rctx.setStatus(HTTP_BAD_REQUEST);
			if (result.isNull()) {
				messages::error("Resource", "Fail to perform append", data::Value(filter->getData()));
			}
		}
	}
}

Resource *ResourceHandler::getResource(Request &rctx) {
	return Resource::resolve(rctx.storage(), _scheme, _subPath, _value);
}

int ResourceHandler::writeDataToRequest(Request &rctx, data::Value &&result) {
	data::Value origin;
	origin.setInteger(_resource->getSourceDelta().toMicroseconds(), "delta");

	if (auto &t = _resource->getQueries().getContinueToken()) {
		if (t.isInit()) {
			data::Value cursor({
				pair("start", data::Value(t.getStart())),
				pair("end", data::Value(t.getEnd())),
				pair("total", data::Value(t.getTotal())),
				pair("count", data::Value(t.getCount())),
				pair("field", data::Value(t.getField())),
			});

			if (t.hasNext()) {
				cursor.setString(t.encodeNext(), "next");
			}

			if (t.hasPrev()) {
				cursor.setString(t.encodePrev(), "prev");
			}

			origin.setValue(move(cursor), "cursor");
		}
	}
	return output::writeResourceData(rctx, move(result), move(origin));
}

int ResourceHandler::writeInfoToReqest(Request &rctx) {
	data::Value result(_resource->getResultObject());
	if (_resource->getType() == ResourceType::File) {
		return output::writeResourceFileHeader(rctx, result);
	}

	return OK;
}

int ResourceHandler::writeToRequest(Request &rctx) {
	if (_resource->getType() == ResourceType::File) {
		data::Value result(_resource->getResultObject());
		if (result) {
			return output::writeResourceFileData(rctx, move(result));
		}
	} else {
		data::Value result(_resource->getResultObject());
		if (result) {
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
