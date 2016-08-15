/*
 * SPIO.h
 *
 *  Created on: 31 марта 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_IO_SPIO_H_
#define COMMON_IO_SPIO_H_

#include "SPCommon.h"
#include "SPIOCommon.h"
#include "SPIOConsumer.h"
#include "SPIOProducer.h"
#include "SPIOBuffer.h"

NS_SP_EXT_BEGIN(io)

size_t read(const Producer &from, const Consumer &to);
size_t read(const Producer &from, const Consumer &to, const Function<void(const Buffer &)> &);
size_t read(const Producer &from, const Consumer &to, const Buffer &);
size_t read(const Producer &from, const Consumer &to, const Buffer &, const Function<void(const Buffer &)> &);

NS_SP_EXT_END(io)

#endif /* COMMON_IO_SPIO_H_ */
