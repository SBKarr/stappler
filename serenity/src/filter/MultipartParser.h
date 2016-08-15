/*
 * MultipartParser.h
 *
 *  Created on: 15 янв. 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_FILTER_MULTIPARTPARSER_H_
#define SERENITY_SRC_FILTER_MULTIPARTPARSER_H_

#include "InputFilter.h"
#include "SPBuffer.h"
#include "SPDataStream.h"

NS_SA_BEGIN

class MultipartParser : public InputParser {
public:
	using Reader = CharReaderBase;

	MultipartParser(const InputConfig &, size_t, const apr::string &);
	MultipartParser(const InputConfig &, size_t, const char *);

	virtual void run(const char *str, size_t len) override;
	virtual void finalize() override;

protected:
	enum class State {
		Begin,
		BeginBlock,
		HeaderLine,
		Data,
		End,
	};

	enum class Data {
		Var,
		File,
		FileAsData,
		Skip,
	};

	enum class Header {
		Begin,
		ContentDisposition,
		ContentDispositionParams,
		ContentDispositionName,
		ContentDispositionFileName,
		ContentDispositionSize,
		ContentDispositionUnknown,
		ContentType,
		ContentEncoding,
		Unknown,
	};

	enum class Literal {
		None,
		Plain,
		Quoted,
	};

	data::Value * flushVarName(Reader &r);
	void flushLiteral(Reader &r, bool quoted);
	void flushData(Reader &r);

	void readBegin(Reader &r);
	void readBlock(Reader &r);
	void readHeaderBegin(Reader &r);
	void readHeaderContentDisposition(Reader &r);
	void readHeaderContentDispositionParam(Reader &r);
	void readHeaderValue(Reader &r);
	void readHeaderDummy(Reader &r);

	void readPlainLiteral(Reader &r);
	void readQuotedLiteral(Reader &r);
	void readHeaderContentDispositionValue(Reader &r);
	void readHeaderContentDispositionDummy(Reader &r);
	void readHeader(Reader &r);
	void readData(Reader &r);

	apr::string name;
	apr::string file;
	apr::string type;
	apr::string encoding;
	size_t size = 0;

	State state = State::Begin;
	Header header = Header::Begin;
	Literal literal = Literal::None;
	Data data = Data::Skip;
	size_t match = 0;
	apr::string boundary;
	Buffer buf;
	data::StreamBuffer dataBuf;
};

NS_SA_END

#endif /* SERENITY_SRC_FILTER_MULTIPARTPARSER_H_ */
