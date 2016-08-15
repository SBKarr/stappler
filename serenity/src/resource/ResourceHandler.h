/*
 * ResourceHandler.h
 *
 *  Created on: 6 марта 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_RESOURCE_RESOURCEHANDLER_H_
#define SERENITY_SRC_RESOURCE_RESOURCEHANDLER_H_

#include "RequestHandler.h"

NS_SA_BEGIN

class ResourceHandler : public RequestHandler {
public:
	ResourceHandler(storage::Scheme *scheme, const data::TransformMap * = nullptr,
			AccessControl * = nullptr, const data::Value & = data::Value());

	virtual bool isRequestPermitted(Request &) override;

	virtual int onTranslateName(Request &) override;
	virtual void onInsertFilter(Request &) override;
	virtual int onHandler(Request &) override;

	virtual void onFilterComplete(InputFilter *f) override;

public: // request interface
	int writeInfoToReqest(Request &rctx);
	int writeToRequest(Request &rctx);

protected:
	void setFileParams(Request &rctx, const data::Value &file);
	void performApiObject(Request &rctx, storage::Scheme *, data::Value &val);
	void performApiFilter(Request &rctx, storage::Scheme *, data::Value &val);

	int writeDataToRequest(Request &rctx, data::Value &objs);
	int getHintedStatus(int) const;

	virtual Resource *getResource(Request &);

	Request::Method _method = Request::Get;
	storage::Scheme *_scheme = nullptr;
	const data::TransformMap * _transform = nullptr;
	AccessControl *_access = nullptr;
	Resource *_resource = nullptr;
	data::Value _value;
};

NS_SA_END

#endif /* SERENITY_SRC_RESOURCE_RESOURCEHANDLER_H_ */
