/*
 * SPDataDecompressBuffer.h
 *
 *  Created on: 11 янв. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_STREAM_SPDATADECOMPRESSBUFFER_H_
#define COMMON_STREAM_SPDATADECOMPRESSBUFFER_H_

#include "SPDataReader.h"
#include "SPData.h"
#include "SPBuffer.h"

NS_SP_EXT_BEGIN(data)

class DecompressBuffer : public AllocBase {
public:
	DecompressBuffer() { }

	size_t read(const uint8_t * s, size_t count) { return count; }

	data::Value & data() { return const_cast<data::Value &>(data::Value::Null); }
	const data::Value & data() const { return data::Value::Null; }

protected:
    Buffer buf;
};

NS_SP_EXT_END(data)

#endif /* COMMON_STREAM_SPDATADECOMPRESSBUFFER_H_ */
