/*
 * Output.h
 *
 *  Created on: 6 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_OUTPUT_OUTPUT_H_
#define SERENITY_SRC_OUTPUT_OUTPUT_H_

#include "Request.h"

NS_SA_EXT_BEGIN(output)

void formatJsonAsHtml(OutputStream &stream, const data::Value &, bool actionHandling = false);

void writeData(Request &rctx, const data::Value &, bool allowJsonP = true);
void writeData(Request &rctx, std::basic_ostream<char> &stream, const Function<void(const String &)> &ct,
		const data::Value &, bool allowJsonP = true);

NS_SA_EXT_END(output)

#endif /* SERENITY_SRC_OUTPUT_OUTPUT_H_ */
