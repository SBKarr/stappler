/*
 * TemplateExec.h
 *
 *  Created on: 24 нояб. 2016 г.
 *      Author: sbkarr
 */

#ifndef APPS_TEMPLATEEXEC_H_
#define APPS_TEMPLATEEXEC_H_

#include "StorageScheme.h"
#include "Resource.h"
#include "TemplateExpression.h"

NS_SA_EXT_BEGIN(tpl)

class Exec : public ReaderClassBase<char> {
public:
	using Callback = Function<void(const String &, Exec &, Request &)>;

	struct Variable {
		const data::Value *value = nullptr;
		storage::Scheme *scheme = nullptr;
	};

	Exec();

	void set(const String &, storage::Scheme *);
	void set(const String &, data::Value &&);
	void set(const String &, data::Value &&, storage::Scheme *);
	void set(const String &, const data::Value *);
	void set(const String &, const data::Value *, storage::Scheme *);
	void clear(const String &);

	Variable getVariable(const CharReaderBase &);

	Variable exec(const Expression &);
	void print(const Expression &, Request &);

	void setIncludeCallback(const Callback &);
	const Callback &getIncludeCallback() const;

	void setTemplateCallback(const Callback &);
	const Callback &getTemplateCallback() const;

protected:
	using ReaderVec = Vector<CharReaderBase>;
	using ReaderVecIt = Vector<CharReaderBase>::iterator;

	Variable selectSchemeSetVariable(ReaderVec &path, ReaderVecIt &pathIt, const Variable &val, const storage::Field *obj);
	Variable selectSchemeByPath(ReaderVec &path, ReaderVecIt &pathIt, storage::Scheme *scheme, int64_t oid = 0, const String &field = String());

	const data::Value * selectDataValue(const data::Value &val, CharReaderBase &r);

	Variable execNode(const Expression::Node *, Expression::Op);
	void printNode(const Expression::Node *, Expression::Op, Request &);

	Variable execPath(ReaderVec &);
	void execPathNode(ReaderVec &, const Expression::Node *);

	Variable execPath(ReaderVec &, ReaderVecIt, Variable &);
	Variable execPathVariable(ReaderVec &, ReaderVecIt, const data::Value *);
	bool execPathObject(ReaderVec &, ReaderVecIt &, Variable &);

	Variable eval(const String &);
	Variable perform(Variable &, Expression::Op);
	Variable perform(Variable &, Variable &, Expression::Op);

	AccessControl _access;
	storage::Adapter *_storage;
	Map<String, Variable> _variables;

	Callback _templateCallback;
	Callback _includeCallback;
};

NS_SA_EXT_END(tpl)

#endif /* APPS_TEMPLATEEXEC_H_ */
