// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "TemplateExec.h"
#include "StorageAdapter.h"
#include "StorageWorker.h"

NS_SA_EXT_BEGIN(tpl)

Exec::Exec() : _storage(storage::Adapter::FromContext()) {
	AccessControl::List list(AccessControl::List::restrictive());
	list.read = AccessControl::Permission::Full;
	_access.setDefaultList(list);
}

void Exec::set(const String &name, const storage::Scheme *scheme) {
	_variables.emplace(name, Variable{nullptr, scheme});
}
void Exec::set(const String &name, data::Value &&val) {
	_variables.emplace(name, Variable{new data::Value(std::move(val)), nullptr});
}

void Exec::set(const String &name, data::Value &&val, const storage::Scheme *scheme) {
	_variables.emplace(name, Variable{new data::Value(std::move(val)), scheme});
}
void Exec::set(const String &name, const data::Value *val) {
	_variables.emplace(name, Variable{val, nullptr});
}
void Exec::set(const String &name, const data::Value *val, const storage::Scheme *scheme) {
	_variables.emplace(name, Variable{val, scheme});
}

void Exec::clear(const String &name) {
	_variables.erase(name);
}

const data::Value * Exec::selectDataValue(const data::Value &val, StringView &r) {
	using namespace chars;
	if (r.empty()) {
		return &val;
	}

	if (r.is('.') && val.isDictionary()) {
		++ r;
		auto tmp = r.readChars<Group<GroupId::Alphanumeric>>();
		auto name = String::make_weak(tmp.data(), tmp.size());

		if (!name.empty()) {
			auto &it = val.getValue(name);
			if (!it.isNull()) {
				return selectDataValue(it, r);
			}
		}
	} else if (r.is('#') && val.isArray()) {
		++ r;
		auto nameRes = r.readInteger();
		if (nameRes.valid()) {
			auto name = nameRes.get();
			if (name >= 0 && size_t(name) < val.size()) {
				auto &it = val.getValue(name);
				if (!it.isNull()) {
					return selectDataValue(it, r);
				}
			}
		}
	}
	return nullptr;
}

Exec::Variable Exec::getVariable(const StringView &ir) {
	StringView r(ir);
	auto tmp = r.readChars<Group<GroupId::Alphanumeric>, Chars<'_'>>();
	auto name = String::make_weak(tmp.data(), tmp.size());
	auto it = _variables.find(name);
	if (it != _variables.end()) {
		return it->second;
	}

	return Variable();
}

Exec::Variable Exec::exec(const Expression &expr) {
	if (expr.root) {
		return execNode(expr.root, Expression::NoOp);
	} else {
		return Variable();
	}
}

void Exec::print(const Expression &expr, Request &rctx) {
	if (expr.root) {
		printNode(expr.root, Expression::NoOp, rctx);
	}
}


void Exec::setIncludeCallback(const Callback &cb) {
	_includeCallback = cb;
}
const Exec::Callback &Exec::getIncludeCallback() const {
	return _includeCallback;
}

void Exec::setTemplateCallback(const Callback &cb) {
	_templateCallback = cb;
}
const Exec::Callback &Exec::getTemplateCallback() const {
	return _templateCallback;
}

void Exec::printNode(const Expression::Node *node, Expression::Op op, Request &rctx) {
	if (node->op != Expression::Comma) { // optimize Comma operator to output results sequentially
		auto var = execNode(node, op);
		if (var.value) {
			const data::Value &val = *var.value;
			switch (val.getType()) {
			case data::Value::Type::CHARSTRING: rctx << val.asString(); break;
			case data::Value::Type::INTEGER: rctx << val.asInteger(); break;
			case data::Value::Type::DOUBLE: rctx << val.asDouble(); break;
			case data::Value::Type::BOOLEAN: rctx << val.asInteger(); break;
			default: rctx << data::EncodeFormat::Json << val; break;
			}
		}
	} else {
		if (node->left) {
			printNode(node->left, Expression::Comma, rctx);
		}
		if (node->right) {
			printNode(node->right, Expression::Comma, rctx);
		}
	}
}

Exec::Variable Exec::execNode(const Expression::Node *node, Expression::Op op) {
	if (!node->value.empty() || node->op == Expression::NoOp) {
		auto ret = eval(node->value);
		if (node->op != Expression::NoOp) {
			ret = perform(ret, node->op);
		}
		return ret;
	}

	if (!node->right && node->left) {
		auto res = execNode(node->left, node->op);
		return perform(res, node->op);
	}

	if (node->right && node->left) {
		if (node->op == Expression::Dot || node->op == Expression::Colon || node->op == Expression::Sharp) {
			Vector<StringView> path;
			execPathNode(path, node);
			return execPath(path);
		} else {
			if (node->op == Expression::And) { // optimize And if first argument is false
				auto l = execNode(node->left, node->op);
				if (l.value && !l.value->asBool()) {
					return l;
				}
				auto r = execNode(node->right, node->op);
				return perform(l, r, node->op);
			} else if (node->op == Expression::Or) { // optimize Or if first argument is true
				auto l = execNode(node->left, node->op);
				if (l.value && l.value->asBool()) {
					return l;
				}
				auto r = execNode(node->right, node->op);
				return perform(l, r, node->op);
			} else {
				auto l = execNode(node->left, node->op);
				auto r = execNode(node->right, node->op);
				return perform(l, r, node->op);
			}
		}
	}

	return Variable();
}

static const char *s_dot_char = ".";
static const char *s_color_char = ":";
static const char *s_sharp_char = "#";

void Exec::execPathNode(ReaderVec &path, const Expression::Node *node) {
	if (!node->value.empty() || node->op == Expression::NoOp) {
		path.push_back(StringView(node->value));
		return;
	}

	if (node->op == Expression::Dot || node->op == Expression::Colon || node->op == Expression::Sharp) {
		execPathNode(path, node->left);
		switch(node->op) {
		case Expression::Dot: path.push_back(StringView(s_dot_char, 1)); break;
		case Expression::Colon: path.push_back(StringView(s_color_char, 1)); break;
		case Expression::Sharp: path.push_back(StringView(s_sharp_char, 1)); break;
		default: break;
		}
		execPathNode(path, node->right);
	} else {
		Variable var = execNode(node, Expression::NoOp);
		if (var.value && var.value->isString()) {
			path.push_back(StringView(var.value->getString()));
		} else {
			const_cast<data::Value *>(var.value)->setString(var.value->asString());
			path.push_back(StringView(var.value->getString()));
		}
	}
}

Exec::Variable Exec::eval(const String &value) {
	StringView r(value);
	if (r.is('$')) {
		++ r;
		return getVariable(r);
	} else if (r.is<Group<GroupId::Numbers>>()) {
		auto tmp = r;
		r.skipUntil<Chars<'e', 'E', '.'>>();
		if (!r.empty()) {
			return Variable{new data::Value(tmp.readFloat().get(0.0f))};
		} else {
			return Variable{new data::Value(tmp.readInteger().get(0))};
		}
	} else if (r.is('\'')) {
		++ r;
		StringStream str;
		while (!r.empty() && !r.is('\'')) {
			str << r.readUntil<Chars<'\'', '\\'>>();
			if (r.is('\\')) {
				++ r;
				if (r.is('r')) {
					str << "\r";
				} else if (r.is('n')) {
					str << "\n";
				} else if (r.is('t')) {
					str << "\t";
				} else {
					str << r.front();
				}
				++ r;
			}
		}
		return Variable{new data::Value(str.str())};
	} else if (r.is('"')) {
		++ r;
		StringStream str;
		while (!r.empty() && !r.is('"')) {
			str << r.readUntil<Chars<'"', '\\'>>();
			if (r.is('\\')) {
				++ r;
				if (r.is('r')) {
					str << "\r";
				} else if (r.is('n')) {
					str << "\n";
				} else if (r.is('t')) {
					str << "\t";
				} else {
					str << r.front();
				}
				++ r;
			}
		}
		return Variable{new data::Value(str.str())};
	} else {
		if (r.is("true")) {
			return Variable{new data::Value(true)};
		} else if (r.is("false")) {
			return Variable{new data::Value(false)};
		} else if (r.is("null")) {
			return Variable{new data::Value()};
		} else if (r.is("inf")) {
			return Variable{new data::Value(std::numeric_limits<double>::infinity())};
		} else if (r.is("nan")) {
			return Variable{new data::Value(nan())};
		}
	}
	return Variable();
}

Exec::Variable Exec::perform(Variable &val, Expression::Op op) {
	Variable ret;

	if (val.value) {
		switch (op) {
		case Expression::Minus:
			ret.value = new data::Value(-val.value->asInteger());
			break;
		case Expression::Neg:
			if (ret.value->isBasicType()) {
				ret.value = new data::Value(!val.value->asBool());
			} else {
				ret.value = new data::Value(val.value->asBool());
			}
			break;
		default:
			break;
		}
	}

	return ret;
}

Exec::Variable Exec::perform(Variable &l, Variable &r, Expression::Op op) {
	Variable ret;

	if (l.value && r.value) {
		switch (op) {
		case Expression::Mult:
			if (l.value->isInteger() && r.value->isInteger()) {
				ret.value = new data::Value(l.value->asInteger() * r.value->asInteger());
			} else {
				ret.value = new data::Value(l.value->asDouble() * r.value->asDouble());
			}
			break;
		case Expression::Div:
			if (l.value->isInteger() && r.value->isInteger()) {
				ret.value = new data::Value(l.value->asInteger() / r.value->asInteger());
			} else {
				ret.value = new data::Value(l.value->asDouble() * r.value->asDouble());
			}
			break;

		case Expression::Sum:
			if (l.value->isString() || r.value->isString()) {
				ret.value = new data::Value(l.value->asString() + r.value->asString());
			} else if (l.value->isInteger() && r.value->isInteger()) {
				ret.value = new data::Value(l.value->asInteger() + r.value->asInteger());
			} else {
				ret.value = new data::Value(l.value->asDouble() + r.value->asDouble());
			}
			break;
		case Expression::Sub:
			if (l.value->isInteger() && r.value->isInteger()) {
				ret.value = new data::Value(l.value->asInteger() - r.value->asInteger());
			} else {
				ret.value = new data::Value(l.value->asDouble() - r.value->asDouble());
			}
			break;

		case Expression::Lt:
			ret.value = new data::Value(l.value->asDouble() < r.value->asDouble());
			break;
		case Expression::ltEq:
			ret.value = new data::Value(l.value->asDouble() <= r.value->asDouble());
			break;
		case Expression::Gt:
			ret.value = new data::Value(l.value->asDouble() > r.value->asDouble());
			break;
		case Expression::GtEq:
			ret.value = new data::Value(l.value->asDouble() >= r.value->asDouble());
			break;

		case Expression::Eq:
			ret.value = new data::Value(*l.value == *r.value);
			break;
		case Expression::NotEq:
			ret.value = new data::Value(*l.value != *r.value);
			break;


		case Expression::And:
			ret.value = new data::Value(l.value->asBool() && r.value->asBool());
			break;
		case Expression::Or:
			ret.value = new data::Value(l.value->asBool() || r.value->asBool());
			break;
		case Expression::Comma:
			ret.value = new data::Value(l.value->asString() + r.value->asString());
			break;
		default:
			break;
		}
	}

	return ret;
}

Exec::Variable Exec::execPath(ReaderVec &path) {
	if (path.empty()) {
		return Variable();
	}

	auto pathIt = path.begin();
	auto r = *pathIt;
	++ pathIt;

	if (!r.is('$')) {
		return Variable();
	}

	++ r;
	auto name = String::make_weak(r.data(), r.size());
	auto it = _variables.find(name);
	if (it != _variables.end()) {
		auto var = it->second;
		if (pathIt == path.end()) {
			return var;
		} else {
			return execPath(path, pathIt, var);
		}
	}

	return Variable();
}

Exec::Variable Exec::execPath(ReaderVec &path, ReaderVecIt pathIt, Variable &var) {
	while (pathIt != path.end()) {
		auto r = *pathIt;
		if (r.is('.') || r.is('#')) {
			if (!var.scheme && var.value) {
				return execPathVariable(path, pathIt, var.value);
			} else if (var.scheme && var.value) {
				if (!execPathObject(path, pathIt, var)) {
					return Variable(); // fail to access object field
				}
			} else {
				return Variable(); // try to access by . or : without value - arror
			}
		} else if (r.is(':')) {
			// access to subcollections (object.subobjects:filter) tracked by execPathObject->selectSchemeSetVariable,
			// so we should only parse collection access
			if (var.scheme && !var.value) {
				var = selectSchemeByPath(path, pathIt, var.scheme);
			} else {
				return Variable();
			}
		} else {
			return Variable();
		}
	}
	return var;
}

Exec::Variable Exec::execPathVariable(ReaderVec &path, ReaderVecIt pathIt, const data::Value *var) {
	while (pathIt != path.end()) {
		auto r = *pathIt;
		++ pathIt;
		if (pathIt == path.end()) {
			return Variable();
		} else if (r.is('.') && var->isDictionary()) {
			auto name = String::make_weak(pathIt->data(), pathIt->size());
			++ pathIt;
			if (!name.empty() && var->hasValue(name)) {
				var = &var->getValue(name);
			} else {
				return Variable();
			}
		} else if (r.is('#') && var->isArray()) {
			r = *pathIt;
			if (!r.readInteger().unwrap([&] (int64_t name) {
				++ pathIt;
				if (name >= 0 && size_t(name) < var->size() && var->hasValue(name)) {
					var = &var->getValue(name);
				}
			})) {
				return Variable();
			}
		} else {
			return Variable();
		}
	}
	return Variable{var};
}

bool Exec::execPathObject(ReaderVec &path, ReaderVecIt &pathIt, Variable &var) {
	auto r = *pathIt;
	++ pathIt;
	if (pathIt == path.end()) {
		return false;
	}
	if (r.is('#') && var.value->isArray()) {
		r = *pathIt;
		++ pathIt;
		if (!r.readInteger().unwrap([&] (int64_t name) {
			if (name >= 0 && size_t(name) < var.value->size() && var.value->hasValue(name)) {
				var.value = &var.value->getValue(name);
			}
		})) {
			return false;
		}

	} else if (r.is('.') && var.value->isDictionary()) {
		auto name = String::make_weak(pathIt->data(), pathIt->size());
		++ pathIt;
		auto f = var.scheme->getField(name);
		if (f) {
			if (f->getType() == storage::Type::Set) {
				var = selectSchemeSetVariable(path, pathIt, var, f);
				if (!var.value) {
					return false;
				}
			} else if (f->getType() == storage::Type::Object || f->getType() == storage::Type::Array
					|| f->getType() == storage::Type::File || f->getType() == storage::Type::Image) {
				var.value = new data::Value(storage::Worker(*var.scheme, _storage).getField(*var.value, *f));
				var.scheme = f->getForeignScheme();
			} else {
				var.value = &var.value->getValue(name);
				var.scheme = nullptr;
			}
		} else if (name == "__oid" && var.value->isInteger("__oid")) {
			var.value = &var.value->getValue("__oid");
			var.scheme = nullptr;
		} else {
			return false;
		}
	}
	return true;
}

Exec::Variable Exec::selectSchemeSetVariable(ReaderVec &path, ReaderVecIt &pathIt, const Variable &val, const storage::Field *obj) {
	data::Value res;
	if (pathIt == path.end() || pathIt->is('.') || pathIt->is('#')) {
		res = storage::Worker(*val.scheme, _storage).getField(*val.value, *obj);
		if (res) {
			return Variable{new data::Value(std::move(res)), obj->getForeignScheme()};
		}
	} else if (pathIt->is(':')) {
		return selectSchemeByPath(path, pathIt, val.scheme, val.value->getInteger("__oid"), obj->getName());
	}

	return Variable();
}

Exec::Variable Exec::selectSchemeByPath(ReaderVec &path, ReaderVecIt &pathIt, const storage::Scheme *scheme, int64_t oid, const StringView &field) {
	StringStream tmp;
	Vector<String> pathComponents; pathComponents.reserve(10);
	if (oid) {
		tmp << "id" << oid;
		pathComponents.emplace_back(tmp.weak());
	}
	if (!field.empty()) {
		pathComponents.emplace_back(field.str());
	}
	while (pathIt != path.end() && pathIt->is(':')) {
		++ pathIt;
		if (pathIt == path.end()) {
			return Variable();
		}
		if (!pathIt->empty()) {
			pathComponents.emplace_back(String::make_weak(pathIt->data(), pathIt->size()));
		} else {
			return Variable();
		}
		++ pathIt;
	}

	std::reverse(pathComponents.begin(), pathComponents.end());
	Resource *res = Resource::resolve(_storage, *scheme, pathComponents);
	if (res) {
		res->setAccessControl(&_access);
		res->prepare();
		return Variable{new data::Value(res->getResultObject()), &res->getScheme()};
	}
	return Variable();
}

NS_SA_EXT_END(tpl)
