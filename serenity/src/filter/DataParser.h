/*
 * DataParser.h
 *
 *  Created on: 20 янв. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_FILTER_DATAPARSER_H_
#define SERENITY_SRC_FILTER_DATAPARSER_H_

#include "InputFilter.h"
#include "SPDataStream.h"

NS_SA_BEGIN

class DataParser : public InputParser {
public:
	DataParser(const InputConfig &c, size_t s) : InputParser(c, s) { }

	virtual void run(const char *str, size_t len) override {
		stream.write(str, len);
	}
	virtual void finalize() override { }

	virtual data::Value &getData() override {
		return stream.data();
	}

protected:
	data::Stream stream;
};

NS_SA_END

#endif /* SERENITY_SRC_FILTER_DATAPARSER_H_ */
