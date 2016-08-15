/*
 * SPNetworkDataTask.h
 *
 *  Created on: 16 марта 2016 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKDATATASK_H_
#define STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKDATATASK_H_

#include "SPNetworkTask.h"
#include "SPDataStream.h"

NS_SP_BEGIN

class NetworkDataTask : public NetworkTask {
public:
	NetworkDataTask();
    virtual ~NetworkDataTask();

    virtual bool init(Method method, const std::string &url, const data::Value &data = data::Value(), data::EncodeFormat = data::EncodeFormat::Cbor);

	const data::Value &getData() const;
	data::Value &getData();
protected:
	data::StreamBuffer _buffer;
};

NS_SP_END

#endif /* STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKDATATASK_H_ */
