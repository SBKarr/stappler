/*
 * SPDataValueNetwork.h
 *
 *  Created on: 10 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_CORE_DATA_SPDATAVALUENETWORK_H_
#define STAPPLER_SRC_CORE_DATA_SPDATAVALUENETWORK_H_

#include "SPData.h"

NS_SP_EXT_BEGIN(data)

void readUrl(const String &, const Function<void(data::Value &)> &, const String &key = "");
void readUrl(const String &, const Function<void(data::Value &)> &, Thread &, const String &key = "");

NS_SP_EXT_END(data)

#endif /* STAPPLER_SRC_CORE_DATA_SPDATAVALUENETWORK_H_ */
