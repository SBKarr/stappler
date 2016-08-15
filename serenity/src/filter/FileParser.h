/*
 * FileParser.h
 *
 *  Created on: 20 янв. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_FILTER_FILEPARSER_H_
#define SERENITY_SRC_FILTER_FILEPARSER_H_

#include "InputFilter.h"

NS_SA_BEGIN

class FileParser : public InputParser {
public:
	FileParser(const InputConfig &c, const apr::string &ct, const apr::string &name, size_t cl)
	: InputParser(c, cl) {
		if (cl < getConfig().maxFileSize) {
			files.emplace_back(String(name), String(ct), String(), String(), cl);
			file = &files.back();
		}
	}

	virtual ~FileParser() { }

	virtual void run(const char *str, size_t len) override {
		if (file && !skip) {
			if (file->writeSize + len < getConfig().maxFileSize) {
				file->write(str, len);
			} else {
				file->close();
				skip = true;
			}
		}
	}
	virtual void finalize() override { }

protected:
	bool skip = false;
	InputFile *file = nullptr;
};

NS_SA_END

#endif /* SERENITY_SRC_FILTER_FILEPARSER_H_ */
