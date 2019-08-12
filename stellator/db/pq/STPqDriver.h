/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_DB_PQ_SPDBPQDRIVER_H_
#define COMMON_DB_PQ_SPDBPQDRIVER_H_

#include "STStorage.h"

NS_DB_PQ_BEGIN

class Driver : public mem::AllocBase {
public:
	using Handle = stappler::ValueWrapper<void *, class HandleClass>;
	using Result = stappler::ValueWrapper<void *, class ResultClass>;
	using Connection = stappler::ValueWrapper<void *, class ConnectionClass>;

	static Driver *open(const mem::StringView &path = mem::StringView());

	enum class Status {
		Empty = 0,
		CommandOk,
		TuplesOk,
		CopyOut,
		CopyIn,
		BadResponse,
		NonfatalError,
		FatalError,
		CopyBoth,
		SingleTuple,
	};

	enum class TransactionStatus {
		Idle,
		Active,
		InTrans,
		InError,
		Unknown
	};

	virtual ~Driver();

	Handle connect(const char * const *keywords, const char * const *values, int expand_dbname) const;
	void finish(Handle) const;

	Connection getConnection(Handle h) const;

	bool isValid(Connection) const;
	TransactionStatus getTransactionStatus(Connection) const;

	Status getStatus(Result res) const;

	bool isBinaryFormat(Result res, size_t field) const;
	bool isNull(Result res, size_t row, size_t field) const;
	char *getValue(Result res, size_t row, size_t field) const;
	size_t getLength(Result res, size_t row, size_t field) const;

	char *getName(Result res, size_t field) const;

	size_t getNTuples(Result res) const;
	size_t getNFields(Result res) const;
	size_t getCmdTuples(Result res) const;

	char *getStatusMessage(Status) const;
	char *getResultErrorMessage(Result res) const;

	void clearResult(Result res) const;

	Result exec(Connection conn, const char *query);
	Result exec(Connection conn, const char *command, int nParams, const char *const *paramValues,
			const int *paramLengths, const int *paramFormats, int resultFormat);

	void release();

protected:
	Driver(const mem::StringView &);

	void *_handle = nullptr;
};

NS_DB_PQ_END

#endif /* COMMON_DB_PQ_SPDBPQDRIVER_H_ */
