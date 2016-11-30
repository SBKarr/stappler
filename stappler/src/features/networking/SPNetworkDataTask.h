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

#ifndef STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKDATATASK_H_
#define STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKDATATASK_H_

#include "SPNetworkTask.h"
#include "SPDataStream.h"

NS_SP_BEGIN

class NetworkDataTask : public NetworkTask {
public:
	NetworkDataTask();
    virtual ~NetworkDataTask();

    virtual bool init(Method method, const String &url, const data::Value &data = data::Value(), data::EncodeFormat = data::EncodeFormat::Cbor);

	const data::Value &getData() const;
	data::Value &getData();
protected:
	data::StreamBuffer _buffer;
};

NS_SP_END

#endif /* STAPPLER_SRC_FEATURES_NETWORKING_SPNETWORKDATATASK_H_ */
