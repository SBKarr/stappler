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

#ifndef STELLATOR_DB_EXTENSIONS_STFIELDINTARRAY_H_
#define STELLATOR_DB_EXTENSIONS_STFIELDINTARRAY_H_

#include "STStorageField.h"

NS_DB_BEGIN

struct FieldIntArray : db::FieldCustom {
	template <typename ... Args>
	FieldIntArray(mem::String && n, Args && ... args) : FieldCustom(std::move(n), std::forward<Args>(args)...) { }

	virtual bool transformValue(const db::Scheme &, const mem::Value &obj, mem::Value &val, bool isCreate) const override;
	virtual mem::Value readFromStorage(const db::ResultCursor &iface, size_t field) const override;
	virtual bool writeToStorage(db::QueryInterface &iface, mem::StringStream &query, const mem::Value &val) const override;
	virtual mem::StringView getTypeName() const override;
	virtual bool isSimpleLayout() const override;
	virtual mem::String getIndexName() const override;
	virtual mem::String getIndexField() const override;

	virtual bool isComparationAllowed(db::Comparation c) const override;

	virtual void writeQuery(const db::Scheme &s, stappler::sql::Query<db::Binder, mem::Interface>::WhereContinue &whi, stappler::sql::Operator op,
			const mem::StringView &f, stappler::sql::Comparation, const mem::Value &val, const mem::Value &) const override;
};

struct FieldBigIntArray : db::FieldCustom {
	template <typename ... Args>
	FieldBigIntArray(mem::String && n, Args && ... args) : FieldCustom(std::move(n), std::forward<Args>(args)...) { }

	virtual bool transformValue(const db::Scheme &, const mem::Value &obj, mem::Value &val, bool isCreate) const override;
	virtual mem::Value readFromStorage(const db::ResultCursor &iface, size_t field) const override ;
	virtual bool writeToStorage(db::QueryInterface &iface, mem::StringStream &query, const mem::Value &val) const override;
	virtual mem::StringView getTypeName() const override;
	virtual bool isSimpleLayout() const override;
	virtual mem::String getIndexName() const override;
	virtual mem::String getIndexField() const override;

	virtual bool isComparationAllowed(db::Comparation c) const override;

	virtual void writeQuery(const db::Scheme &s, stappler::sql::Query<db::Binder, mem::Interface>::WhereContinue &whi, stappler::sql::Operator op,
			const mem::StringView &f, stappler::sql::Comparation, const mem::Value &val, const mem::Value &) const override;
};

NS_DB_END

#endif /* STELLATOR_DB_EXTENSIONS_STFIELDINTARRAY_H_ */
