/*
 * InputFile.h
 *
 *  Created on: 24 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_FILTER_INPUTFILE_H_
#define SERENITY_SRC_FILTER_INPUTFILE_H_

#include "Define.h"

NS_SA_BEGIN

struct InputFile : public AllocPool {
	apr::string path;
	apr::string name;
	apr::string type;
	apr::string encoding;
	apr::string original;
	apr::file file;

	size_t writeSize;
	size_t headerSize;

	InputFile(apr::string && name, apr::string && type, apr::string && enc, apr::string && orig, size_t s);
	~InputFile();

	bool isOpen() const;

	size_t write(const char *, size_t);
	void close();

	bool save(const apr::string &) const;

	InputFile(const InputFile &) = delete;
	InputFile(InputFile &&) = delete;

	InputFile &operator=(const InputFile &) = delete;
	InputFile &operator=(InputFile &&) = delete;
};

NS_SA_END

#endif /* SERENITY_SRC_FILTER_INPUTFILE_H_ */
