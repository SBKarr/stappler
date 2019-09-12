/**
 Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPugContext.h"

#include "SPugVariable.h"

NS_SP_EXT_BEGIN(pug)

struct ContextFn {
	using VarScope = Context::VarScope;

	template <typename Callback>
	bool extractValue(Var &var, const Callback &cb);

	template <typename Callback>
	bool extractName(Var &var, const Callback &cb);

	Var execOperator(const Expression &expr, Expression::Op op);
	Var execComposition(const Expression &expr, Expression::Op op);

	size_t prepareArgsList(Context::VarList &, const Expression &expr, Expression::Block block);

	Var performVarExpr(const Expression &expr, Expression::Op op);
	Var performUnaryOp(Var &v, Expression::Op op);
	Var performBinaryOp(Var &l, Var &r, Expression::Op op);

	Var execExpr(const Expression &expr, Expression::Op op, bool assignable);
	bool printExpr(const Expression &expr, Expression::Op op, std::ostream &out);

	Value *getMutableValue(Var &val) const;
	void onError(const StringView &) const;

	Var getVar(const StringView &key) const;
	Var getVar(VarScope *, const StringView &key) const;

	const VarStorage *getVarStorage(VarScope *, const StringView &key) const;

	VarScope *currentScope = nullptr;
	std::ostream *outStream = nullptr;
	bool escapeOutput = false;
	bool allowUndefined = false;
};

template <typename Callback>
bool ContextFn::extractValue(Var &var, const Callback &cb) {
	switch (var.type) {
	case Var::Temporary:
		switch (var.temporaryStorage.type) {
		case VarStorage::ValueReference:
			if (auto mut = var.temporaryStorage.getMutable()) {
				return cb(move(*mut));
			} else {
				return cb(Value(var.temporaryStorage.readValue()));
			}
			break;
		default:
			onError("Dereference of non-value type is forbidden");
			break;
		}
		break;
	case Var::Static:
		if (auto mut = var.staticStorage.getMutable()) {
			return cb(move(*mut));
		} else {
			return cb(Value(var.staticStorage.readValue()));
		}
		break;
	case Var::Variable:
		switch (var.variableStorage->type) {
		case VarStorage::ValueReference:
			return cb(Value(var.variableStorage->readValue()));
			break;
		default:
			onError("Dereference of non-value type is forbidden");
			break;
		}
		break;
	default:
		onError("Dereference of undefined is forbidden");
		break;
	}
	return false;
}

template <typename Callback>
bool ContextFn::extractName(Var &var, const Callback &cb) {
	switch (var.type) {
	case Var::Temporary:
		switch (var.temporaryStorage.type) {
		case VarStorage::ValueReference:
			return cb(var.temporaryStorage.readValue().asString());
			break;
		default:
			onError("Dereference of non-value type is forbidden");
			break;
		}
		break;
	case Var::Static:
		return cb(var.staticStorage.readValue().asString());
		break;
	case Var::Variable:
		switch (var.variableStorage->type) {
		case VarStorage::ValueReference:
			return cb(var.variableStorage->readValue().asString());
			break;
		default:
			onError("Dereference of non-value type is forbidden");
			break;
		}
		break;
	default:
		onError("Dereference of undefined is forbidden");
		break;
	}
	return false;
}


Var ContextFn::execOperator(const Expression &expr, Expression::Op op) {
	Context::VarList list;
	if (prepareArgsList(list, expr, Expression::Operator) != maxOf<size_t>()) {
		auto val = new Value(Value::Type::DICTIONARY);
		auto &dict = val->asDict();
		auto d = list.data();
		auto s = list.size();
		dict.reserve(s / 2);
		for (size_t i = 0; i < s / 2; ++ i) {
			if (!extractName(d[i * 2], [&] (const StringView &name) {
				if (!extractValue(d[i * 2 + 1], [&] (Value &&val) {
					dict.emplace(name.str<memory::PoolInterface>(), move(val));
					return true;
				})) {
					return false;
				}
				return true;
			})) {
				return Var();
			}
		}
		return Var(false, val);
	} else {
		onError("Fail to read argument list for { } expression");
	}
	return Var();
}

Var ContextFn::execComposition(const Expression &expr, Expression::Op op) {
	Context::VarList list;
	if (prepareArgsList(list, expr, Expression::Composition) != maxOf<size_t>()) {
		auto val = new Value(Value::Type::ARRAY);
		auto &arr = val->asArray();
		auto d = list.data();
		auto s = list.size();
		arr.reserve(s);
		for (size_t i = 0; i < s; ++ i) {
			if (!extractValue(d[i], [&] (Value &&val) {
				arr.emplace_back(move(val));
				return true;
			})) {
				return Var();
			}
		}
		return Var(false, val);
	} else {
		onError("Fail to read argument list for [ ] expression");
	}
	return Var();
}

size_t ContextFn::prepareArgsList(Context::VarList &list, const Expression &expr, Expression::Block block) {
	if (block == Expression::Parentesis || block == Expression::Composition) {
		if (expr.op == Expression::Comma && expr.left && expr.right) {
			auto r1 = prepareArgsList(list, *expr.left, block);
			auto r2 = prepareArgsList(list, *expr.right, block);
			if (r1 != maxOf<size_t>() && r2 != maxOf<size_t>()) {
				return r1 + r2;
			}
		} else if (!expr.left && !expr.right && expr.op != Expression::NoOp) {
			return 0;
		} else {
			if (expr.isToken && expr.value.empty()) {
				return 0;
			}
			if (auto var = execExpr(expr, (block == Expression::Parentesis) ? Expression::Call : Expression::Subscript, false)) {
				list.emplace(move(var));
				return 1;
			}
		}
	} else if (block == Expression::Operator) {
		if ((expr.op == Expression::Comma || expr.op == Expression::Sequence) && expr.left && expr.right) {
			auto r1 = prepareArgsList(list, *expr.left, block);
			auto r2 = prepareArgsList(list, *expr.right, block);
			if (r1 != maxOf<size_t>() && r2 != maxOf<size_t>()) {
				return r1 + r2;
			}
		} else {
			if (expr.op == Expression::Colon && expr.left && expr.right) {
				if (auto key = execExpr(*expr.left, Expression::Colon, false)) {
					if (auto value = execExpr(*expr.right, Expression::Construct, false)) {
						list.emplace(move(key));
						list.emplace(move(value));
						return 1;
					}
				}
			} else if (!expr.left && !expr.right && expr.op != Expression::NoOp) {
				return 0;
			}
		}
	}
	return maxOf<size_t>();
}

static bool ContextFn_asBool(const data::Value &v) {
	return v.isBasicType() ? v.asBool() : !v.empty();
}

Var ContextFn::execExpr(const Expression &expr, Expression::Op op, bool assignable) {
	if (expr.left || expr.right || expr.op != Expression::NoOp) {
		if (expr.block == Expression::Operator && op != Expression::Construct) {
			return execOperator(expr, op);
		} else if (expr.block == Expression::Composition && op != Expression::Subscript) {
			return execComposition(expr, op);
		}
	}

	if (!expr.left && !expr.right) {
		if (expr.isToken && op != Expression::Colon) {
			return getVar(expr.value.getString());
		} else {
			return Var(true, &expr.value);
		}
	} else if (expr.left && expr.right) {
		switch (expr.op) {
		case Expression::Call:
			if (auto var = execExpr(*expr.left, expr.op, assignable)) {
				if (auto cb = var.getCallable()) {
					Context::VarList list;
					if (prepareArgsList(list, *expr.right, Expression::Parentesis) != maxOf<size_t>()) {
						if (auto ret = (*cb)(*var.getStorage(), list.data(), list.size())) {
							return ret;
						} else {
							onError("Fail to make call");
							return Var();
						}
					} else {
						onError("Invalid argument list for call");
						return Var();
					}
				} else {
					onError("Expression is not callable");
					return Var();
				}
			}
			break;
		case Expression::Subscript:
			if (auto var = execExpr(*expr.left, expr.op, assignable)) {
				if (var.getType() == Var::SoftUndefined) {
					onError("Invalid subscription [] from <undefined>");
					return Var();
				}
				Context::VarList list;
				if (prepareArgsList(list, *expr.right, Expression::Composition) == 1) {
					if (auto &val = list.data()->readValue()) {
						if (val.isInteger()) {
							if (auto ret = var.subscript(val.asInteger(), assignable)) {
								return ret;
							} else {
								return Var(nullptr);
							}
						} else if (!val.isDictionary() && !val.isArray()) {
							auto str = val.asString();
							if (auto ret = var.subscript(str, assignable)) {
								return ret;
							} else {
								return Var(nullptr);
							}
						}
					}
				}
				onError("Invalid subscription [] arguments");
				return Var();
			}
			break;
		case Expression::Construct:
			onError("Construct not implemented");
			return Var();
			break;
		case Expression::And:
			if (auto var = execExpr(*expr.left, expr.op, assignable)) {
				if (!ContextFn_asBool(var.readValue())) {
					return var;
				}
			} else {
				return Var();
			}
			return execExpr(*expr.right, expr.op, assignable);
			break;
		case Expression::Or:
			if (auto var = execExpr(*expr.left, expr.op, assignable)) {
				if (ContextFn_asBool(var.readValue())) {
					return var;
				}
			} else {
				return Var();
			}
			return execExpr(*expr.right, expr.op, assignable);
			break;
		case Expression::ConditionalSwitch:
			if (expr.left && expr.left->op == Expression::Conditional && expr.left->left) {
				auto tmpUndef = allowUndefined;
				allowUndefined = true;
				if (auto var = execExpr(*expr.left->left, expr.left->op, assignable)) {
					allowUndefined = tmpUndef;
					if (ContextFn_asBool(var.readValue())) {
						return execExpr(*expr.left->right, expr.op, assignable);
					} else {
						return execExpr(*expr.right, expr.op, assignable);
					}
				} else {
					allowUndefined = tmpUndef;
					return execExpr(*expr.right, expr.op, assignable);
				}
			}
			onError("Invalid conditional expression");
			return Var();
			break;
		case Expression::Assignment:
		case Expression::SumAssignment:
		case Expression::DiffAssignment:
		case Expression::MultAssignment:
		case Expression::DivAssignment:
		case Expression::RemAssignment:
		case Expression::LShiftAssignment:
		case Expression::RShiftAssignment:
		case Expression::AndAssignment:
		case Expression::XorAssignment:
		case Expression::OrAssignment: { // left-to-right associativity
			auto r = execExpr(*expr.right, expr.op, assignable);
			auto l = execExpr(*expr.left, expr.op, true);
			return performBinaryOp(l, r, expr.op);
			break;
		}
		case Expression::Dot: {
			auto l = execExpr(*expr.left, expr.op, assignable);
			if (l && l.getType() != Var::SoftUndefined) {
				if (expr.right && expr.right->isToken && expr.right->value.isString()) {
					if (auto v = l.subscript(expr.right->value.getString(), assignable)) {
						return v;
					} else {
						return Var(nullptr);
					}
				} else {
					onError(toString("Invalid argument for <dot> operator"));
					return Var();
				}
			} else {
				onError(toString("Fail to read <dot>: <undefined>.", expr.right->value.getString()));
				return Var();
			}
			break;
		}
		default: { // right-to-left associativity
			auto l = execExpr(*expr.left, expr.op, assignable);
			auto r = execExpr(*expr.right, expr.op, assignable);
			return performBinaryOp(l, r, expr.op);
			break;
		}
		}
	} else if (expr.left) {
		if (expr.op == Expression::Neg) {
			auto var = execExpr(*expr.left, expr.op, assignable);
			if (var) {
				return performUnaryOp(var, expr.op);
			} else {
				onError(toString("Invalid call <neg> <undefined>"));
				return Var(Value(true));
			}
		} else if (expr.op != Expression::Var) {
			if (auto var = execExpr(*expr.left, expr.op, assignable)) {
				return performUnaryOp(var, expr.op);
			}
		} else {
			return performVarExpr(*expr.left, expr.op);
		}
	}

	return Var();
}

static void ContextFn_printEscapedString(const StringView &str, std::ostream &out, bool escapeOutput) {
	if (escapeOutput) {
		StringView r(str);
		while (!r.empty()) {
			out << r.readUntil<StringView::Chars<'&', '<', '>', '"', '\''>>();
			switch (r[0]) {
			case '&': out << "&amp;"; break;
			case '<': out << "&lt;"; break;
			case '>': out << "&gt;"; break;
			case '"': out << "&quot;"; break;
			case '\'': out << "&#39;"; break;
			}
			++ r;
		}
	} else {
		out << str;
	}
}

static void ContextFn_printVar(const Var &var, std::ostream &out, bool escapeOutput) {
	auto &val = var.readValue();
	switch (val.getType()) {
	case Value::Type::CHARSTRING:
		ContextFn_printEscapedString(val.getString(), out, escapeOutput);
		break;
	case Value::Type::INTEGER: out << val.asInteger(); break;
	case Value::Type::DOUBLE: out << val.asDouble(); break;
	case Value::Type::BOOLEAN: out << (val.asBool() ? "true" : "false"); break;
	case Value::Type::EMPTY: out << "null"; break;
	default: out << data::EncodeFormat::Json << val; break;
	}
}

bool ContextFn::printExpr(const Expression &expr, Expression::Op op, std::ostream &out) {
	 // optimize Comma operator to output results sequentially
	if (expr.op != Expression::Comma || (expr.block != Expression::None && expr.block != Expression::Parentesis)) {
		if (auto var = execExpr(expr, op, false)) {
			ContextFn_printVar(var, out, escapeOutput);
			return true;
		}
	} else {
		if (expr.left) {
			if (!printExpr(*expr.left, Expression::Comma, out)) {
				return false;
			}
		}
		if (expr.right) {
			if (!printExpr(*expr.right, Expression::Comma, out)) {
				return false;
			}
		}
		return true;
	}
	return false;
}

Var ContextFn::performVarExpr(const Expression &expr, Expression::Op op) {
	if (expr.isToken && expr.value.isString()) {
		auto &n = expr.value.getString();
		auto it = currentScope->namedVars.find(n);
		if (it == currentScope->namedVars.end()) {
			auto p_it = currentScope->namedVars.emplace(n, VarStorage());
			return Var(&p_it.first->second);
		} else {
			onError(toString("Variable name conflict for ", n));
		}
	}
	return Var();
}

Var ContextFn::performUnaryOp(Var &v, Expression::Op op) {
	if (auto &val = v.readValue()) {
		switch (op) {
		case Expression::SuffixIncr:
			if (auto mut = getMutableValue(v)) {
				auto i = val.asInteger();
				*mut = Value(i + 1);
				return Var(Value(i));
			} else {
				return Var();
			}
			break;

		case Expression::SuffixDecr:
			if (auto mut = getMutableValue(v)) {
				auto i = val.asInteger();
				*mut = Value(i - 1);
				return Var(Value(i));
			} else {
				onError(toString("Fail to write into constant value"));
				return Var();
			}
			break;

		case Expression::PrefixIncr:
			if (auto mut = getMutableValue(v)) {
				*mut = Value(val.asInteger() + 1);
				return v;
			} else {
				onError(toString("Fail to write into constant value"));
				return Var();
			}
			break;

		case Expression::PrefixDecr:
			if (auto mut = getMutableValue(v)) {
				*mut = Value(val.asInteger() - 1);
				return v;
			} else {
				onError(toString("Fail to write into constant value"));
				return Var();
			}
			break;

		case Expression::Minus:
			return Var(val.isInteger() ? Value(- val.asInteger()) : Value(- val.asDouble()));
			break;

		case Expression::Neg:
			return Var(val.isBasicType() ? Value(!val.asBool()) : Value(!val.empty()));
			break;

		case Expression::BitNot:
			return Var(Value(~ val.asInteger()));
			break;

		default:
			onError(toString("Invalid unary operator: ", int(toInt(op))));
			return Var();
			break;
		}
	} else if (allowUndefined) {
		switch (op) {
		case Expression::Neg:
			return Var(Value(true));
			break;
		default: break;
		}
	}

	onError("Unexpected null value on unary op");
	return Var();
}

template <typename Callback>
static bool Variable_assign(Var &target, const Var &var, Callback &&cb) {
	if (auto mut = target.getMutable()) {
		if (cb(*mut, var.readValue())) {
			return true;
		}
	}
	return false;
}

Var ContextFn::performBinaryOp(Var &l, Var &r, Expression::Op op) {
	auto &lV = l.readValue();
	auto &rV = r.readValue();

	auto numOp = [&] (const Value &l, const Value &r, auto intOp, auto doubleOp) {
		if (l.isInteger() && r.isInteger()) {
			return Var(Value(intOp(l.asInteger(), r.asInteger())));
		} else if (l.isBasicType() && r.isBasicType()) {
			return Var(Value(doubleOp(l.asDouble(), r.asDouble())));
		} else {
			onError("Invalid type for numeric operation");
			return Var();
		}
	};

	auto intOp = [&] (const Value &l, const Value &r, auto intOp) {
		if (l.isInteger() && r.isInteger()) {
			return Var(Value(intOp(l.asInteger(), r.asInteger())));
		} else {
			onError("Invalid type for integer operation");
			return Var();
		}
	};

	auto numberAssignment = [&] (auto intOp, auto doubleOp) -> Var {
		if (Variable_assign(l, r, [&] (Value &mut, const Value &r) -> bool {
			if ((mut.isInteger() || mut.isDouble()) && (r.isInteger() || r.isDouble())) {
				if (mut.isInteger() && r.isInteger()) {
					mut.setInteger(intOp(mut.getInteger(), r.asInteger()));
				} else {
					mut.setDouble(doubleOp(mut.asDouble(), r.asDouble()));
				}
				return true;
			}
			return false;
		})) {
			return Var(l);
		} else {
			onError("Invalid assignment operator");
			return Var();
		}
	};

	auto intAssignment = [&] (auto intOp) -> Var {
		if (Variable_assign(l, r, [&] (Value &mut, const Value &r) -> bool {
			if (mut.isInteger()  && r.isInteger()) {
				mut.setInteger(intOp(mut.getInteger(), r.asInteger()));
				return true;
			}
			return false;
		})) {
			return Var(l);
		} else {
			onError("Invalid assignment operator");
			return Var();
		}
	};

	auto variableAssign = [&] (Var &target, const Var &var) -> bool {
		return target.assign(var);
	};

	// try assignment
	if (r.getType() != Var::SoftUndefined) {
		switch (op) {
		case Expression::Assignment:
			if (variableAssign(l, r)) {
				return Var(l);
			} else {
				onError("Invalid assignment operator");
				return Var();
			}
			break;
		case Expression::SumAssignment:
			if (Variable_assign(l, r, [this] (Value &mut, const Value &r) -> bool {
				if (!mut) { mut = r; return true; }
				else if (mut.isString()) { mut.getString() += r.asString(); return true; }
				else if (mut.isInteger() && !r.isDouble()) { mut.setInteger(mut.getInteger() + r.asInteger()); return true; }
				else if ((mut.isInteger() && r.isDouble()) || mut.isDouble()) { mut.setDouble(mut.asDouble() + r.asDouble()); return true; }
				else if (mut.isBool()) { mut.setBool(mut.asBool() + r.asBool()); return true; }
				else { onError("Invalid SumAssignment operator"); return false; }
			})) {
				return Var(l);
			} else {
				onError("Invalid assignment operator");
				return Var();
			}
			break;
		case Expression::DiffAssignment: return numberAssignment([] (int64_t l, int64_t r) { return l - r; }, [] (double l, double r) { return l - r; }); break;
		case Expression::MultAssignment: return numberAssignment([] (int64_t l, int64_t r) { return l * r; }, [] (double l, double r) { return l * r; }); break;
		case Expression::DivAssignment: return numberAssignment([] (int64_t l, int64_t r) { return l / r; }, [] (double l, double r) { return l / r; }); break;
		case Expression::RemAssignment: return intAssignment([] (int64_t l, int64_t r) { return l % r; }); break;
		case Expression::LShiftAssignment: return intAssignment([] (int64_t l, int64_t r) { return l << r; }); break;
		case Expression::RShiftAssignment: return intAssignment([] (int64_t l, int64_t r) { return l >> r; }); break;
		case Expression::AndAssignment: return intAssignment([] (int64_t l, int64_t r) { return l & r; }); break;
		case Expression::XorAssignment: return intAssignment([] (int64_t l, int64_t r) { return l ^ r; }); break;
		case Expression::OrAssignment: return intAssignment([] (int64_t l, int64_t r) { return l | r; }); break;
		default: break;
		}
	}

	if (lV && rV) {
		switch (op) {
		case Expression::Mult: return numOp(lV, rV, [] (int64_t l, int64_t r) { return l * r; }, [] (double l, double r) { return l * r; }); break;
		case Expression::Div: return numOp(lV, rV, [] (int64_t l, int64_t r) { return l / r; }, [] (double l, double r) { return l / r; }); break;
		case Expression::Rem: return intOp(lV, rV, [] (int64_t l, int64_t r) { return l % r; }); break;

		case Expression::Sum:
			if (lV.isString() || rV.isString()) {
				return Var(Value(lV.asString() + rV.asString()));
			} else if (lV.isInteger() && rV.isInteger()) {
				return Var(Value(lV.asInteger() + rV.asInteger()));
			} else if (lV.isDictionary() && rV.isDictionary()) {
				auto d = lV.asDict();
				for (auto &it : rV.asDict()) {
					d.emplace(it.first, it.second);
				}
				return Var(false, new Value(std::move(d)));
			} else if (lV.isArray() && rV.isArray()) {
				auto d = lV.asArray();
				for (auto &it : rV.asArray()) {
					d.emplace_back(it);
				}
				return Var(false, new Value(std::move(d)));
			} else {
				return Var(Value(lV.asDouble() + rV.asDouble()));
			}
			break;

		case Expression::Sub: return numOp(lV, rV, [] (int64_t l, int64_t r) { return l - r; }, [] (double l, double r) { return l - r; }); break;

		case Expression::Lt: return numOp(lV, rV, [] (int64_t l, int64_t r) { return l < r; }, [] (double l, double r) { return l < r; }); break;
		case Expression::LtEq: return numOp(lV, rV, [] (int64_t l, int64_t r) { return l <= r; }, [] (double l, double r) { return l <= r; }); break;
		case Expression::Gt: return numOp(lV, rV, [] (int64_t l, int64_t r) { return l > r; }, [] (double l, double r) { return l > r; }); break;
		case Expression::GtEq: return numOp(lV, rV, [] (int64_t l, int64_t r) { return l >= r; }, [] (double l, double r) { return l >= r; }); break;

		case Expression::Eq: return Var(Value(lV == rV)); break;
		case Expression::NotEq: return Var(Value(lV != rV)); break;

		case Expression::And: return Var(Value(ContextFn_asBool(lV) && ContextFn_asBool(rV))); break;
		case Expression::Or: return Var(Value(ContextFn_asBool(lV) || ContextFn_asBool(rV))); break;
		case Expression::Comma: return Var(Value(lV.asString() + rV.asString())); break;

		case Expression::ShiftLeft: return intOp(lV, rV, [] (int64_t l, int64_t r) { return l << r; }); break;
		case Expression::ShiftRight: return intOp(lV, rV, [] (int64_t l, int64_t r) { return l >> r; }); break;
		case Expression::BitAnd: return intOp(lV, rV, [] (int64_t l, int64_t r) { return l & r; }); break;
		case Expression::BitOr: return intOp(lV, rV, [] (int64_t l, int64_t r) { return l | r; }); break;
		case Expression::BitXor: return intOp(lV, rV, [] (int64_t l, int64_t r) { return l ^ r; }); break;

		case Expression::Dot:
		case Expression::Var:
		case Expression::Sharp:
		case Expression::Scope:

		case Expression::Call:
		case Expression::Subscript:
		case Expression::Construct:

		case Expression::Colon:
		case Expression::Sequence:

		case Expression::Conditional:
		case Expression::ConditionalSwitch:
		case Expression::Assignment:
		case Expression::SumAssignment:
		case Expression::DiffAssignment:
		case Expression::MultAssignment:
		case Expression::DivAssignment:
		case Expression::RemAssignment:
		case Expression::LShiftAssignment:
		case Expression::RShiftAssignment:
		case Expression::AndAssignment:
		case Expression::XorAssignment:
		case Expression::OrAssignment:
			onError(toString("Operator not implemented: ", int(toInt(op))));
			return Var();
			break;

		case Expression::NoOp:
			onError(toString("Call of NoOp"));
			return Var();
			break;

		case Expression::SuffixIncr:
		case Expression::SuffixDecr:
		case Expression::PrefixIncr:
		case Expression::PrefixDecr:
		case Expression::Minus:
		case Expression::Neg:
		case Expression::BitNot:
			onError(toString("Unary operator called as binary: ", int(toInt(op))));
			return Var();
			break;
		}

		return Var();
	} else if (lV || rV) {
		auto valToInt = [&] (const Value &v, const Var &var) {
			return (v) ? v.asInteger() : int64_t(0);
		};

		auto valToDouble = [&] (const Value &v, const Var &var) {
			return (v) ? v.asInteger() : ((var.getType() == Var::SoftUndefined) ? std::numeric_limits<double>::quiet_NaN() : double(0.0));
		};

		auto numNullOp = [&] (const Value &lVal, const Value &rVal, auto intOp, auto doubleOp) {
			if (lVal.isInteger() || rVal.isInteger()) {
				return Var(Value(intOp(valToInt(lVal, l), valToInt(rVal, r))));
			} else if (lVal.isBasicType() && rVal.isBasicType()) {
				return Var(Value(doubleOp(valToDouble(lVal, l), valToDouble(rVal, r))));
			} else {
				onError("Invalid type for numeric operation");
				return Var();
			}
		};

		switch (op) {
		case Expression::Lt: return numNullOp(lV, rV, [] (int64_t l, int64_t r) { return l < r; }, [] (double l, double r) { return l < r; }); break;
		case Expression::LtEq: return numNullOp(lV, rV, [] (int64_t l, int64_t r) { return l <= r; }, [] (double l, double r) { return l <= r; }); break;
		case Expression::Gt: return numNullOp(lV, rV, [] (int64_t l, int64_t r) { return l > r; }, [] (double l, double r) { return l > r; }); break;
		case Expression::GtEq: return numNullOp(lV, rV, [] (int64_t l, int64_t r) { return l >= r; }, [] (double l, double r) { return l >= r; }); break;

		case Expression::Eq: return Var(Value(lV.isNull() && rV.isNull())); break;
		case Expression::NotEq: return Var(Value(!lV.isNull() || !rV.isNull())); break;

		case Expression::And: return Var(Value(false)); break;
		case Expression::Or: return Var(Value(ContextFn_asBool(lV) || ContextFn_asBool(rV))); break;
		default:
			onError(toString("Invalid operation with undefined"));
			return Var();
			break;
		}
	}
	return Var();
}

Value *ContextFn::getMutableValue(Var &val) const {
	if (auto v = val.getMutable()) {
		return v;
	}
	onError("Fail to acquire mutable value from constant");
	return nullptr;
}

void ContextFn::onError(const StringView &err) const {
	if (!outStream) {
		std::cout << "Context error: " << err << "\n";
	} else {
		*outStream << "<!-- " << "Context error: " << err << " -->";
	}
}

Var ContextFn::getVar(const StringView &key) const {
	Var ret;
	if (currentScope) {
		ret = getVar(currentScope, key);
	}
	if (!ret && !allowUndefined) {
		onError(toString("Access to undefined variable: ", key));
	}

	if (!ret && allowUndefined) {
		return Var(nullptr);
	}

	return ret;
}

Var ContextFn::getVar(VarScope *scope, const StringView &key) const {
	auto it = scope->namedVars.find(key);
	if (it != scope->namedVars.end()) {
		return Var(&it->second);
	} else if (scope->prev) {
		return getVar(scope->prev, key);
	} else {
		return Var();
	}
}

const VarStorage *ContextFn::getVarStorage(VarScope *scope, const StringView &key) const {
	auto it = scope->namedVars.find(key);
	if (it != scope->namedVars.end()) {
		return &it->second;
	} else if (scope->prev) {
		return getVarStorage(scope->prev, key);
	}
	return nullptr;
}

void Context::VarList::emplace(Var &&var) {
	if (staticCount < MinStaticVars) {
		staticList[staticCount] = move(var);
		++ staticCount;
	} else if (staticCount == MinStaticVars) {
		dynamicList.reserve(MinStaticVars * 2);
		for (auto &it : staticList) {
			dynamicList.emplace_back(move(it));
			it.clear();
		}
		dynamicList.emplace_back(move(var));
		staticCount = MinStaticVars + 1;
	} else {
		dynamicList.emplace_back(move(var));
	}
}

size_t Context::VarList::size() const {
	return (staticCount <= MinStaticVars) ? staticCount : dynamicList.size();
}

Var * Context::VarList::data() {
	return (staticCount <= MinStaticVars) ? staticList.data() : dynamicList.data();
}


bool Context::isConstExpression(const Expression &expr) {
	return expr.isConst();
}

bool Context::printConstExpr(const Expression &expr, std::ostream &out, bool escapeOutput) {
	ContextFn fn;
	fn.outStream = &out;
	fn.escapeOutput = escapeOutput;
	return fn.printExpr(expr, expr.op, out);
}

static void Context_printAttrVar(const StringView &name, const Var &var, std::ostream &out, bool escapeOutput) {
	auto &val = var.readValue();
	if (val.isBool()) {
		if (val.getBool()) {
			out << " " << name;
		}
	} else if (val.isArray()) {
		out << " " << name << "=\"";
		bool empty = true;
		for (auto &it : val.asArray()) {
			if (empty) { empty = false; } else { out << " "; }
			ContextFn_printEscapedString(it.asString(), out, escapeOutput);
		}
		out << "\"";
	} else if (val.isDictionary() && name == "style") {
		out << " " << name << "=\"";
		for (auto &it : val.asDict()) {
			out << it.first << ":";
			ContextFn_printEscapedString(it.second.asString(), out, escapeOutput);
			out << ";";
		}
		out << "\"";
	} else {
		out << " " << name << "=\"";
		ContextFn_printVar(var, out, escapeOutput);
		out << "\"";
	}
}

static void Context_printAttrList(const Var &var, std::ostream &out) {
	auto &val = var.readValue();
	for (auto &it : val.asDict()) {
		out << " " << it.first << "=\"" << it.second.asString() << "\"";
	}
}

bool Context::printAttrVar(const StringView &name, const Expression &expr, std::ostream &out, bool escapeOutput) {
	ContextFn fn;
	fn.outStream = &out;
	fn.escapeOutput = escapeOutput;
	if (auto var = fn.execExpr(expr, expr.op, false)) {
		Context_printAttrVar(name, var, out, escapeOutput);
		return true;
	}
	return false;
}

bool Context::printAttrExpr(const Expression &expr, std::ostream &out) {
	ContextFn fn;
	fn.outStream = &out;
	fn.escapeOutput = false;
	if (auto var = fn.execExpr(expr, expr.op, false)) {
		Context_printAttrList(var, out);
		return true;
	}
	return false;
}

Context::Context() : currentScope(&globalScope) { }

bool Context::print(const Expression &expr, std::ostream &out, bool escapeOutput) {
	ContextFn fn{currentScope};
	fn.outStream = &out;
	fn.escapeOutput = escapeOutput;
	return fn.printExpr(expr, expr.op, out);
}

bool Context::printAttr(const StringView &name, const Expression &expr, std::ostream &out, bool escapeOutput) {
	ContextFn fn{currentScope};
	fn.outStream = &out;
	fn.escapeOutput = escapeOutput;
	if (auto var = fn.execExpr(expr, expr.op, false)) {
		Context_printAttrVar(name, var, out, escapeOutput);
		return true;
	}
	return false;
}

bool Context::printAttrExprList(const Expression &expr, std::ostream &out) {
	ContextFn fn{currentScope};
	fn.outStream = &out;
	fn.escapeOutput = false;
	if (auto var = fn.execExpr(expr, expr.op, false)) {
		Context_printAttrList(var, out);
		return true;
	}
	return false;
}

Var Context::exec(const Expression &expr, std::ostream &out, bool allowUndefined) {
	ContextFn fn{currentScope};
	fn.outStream = &out;
	fn.allowUndefined = allowUndefined;
	return fn.execExpr(expr, expr.op, false);
}

void Context::set(const StringView &name, const Value &val, VarClass *cl) {
	auto it = currentScope->namedVars.emplace(name.str<memory::PoolInterface>()).first;
	it->second.set(val, cl);
}

void Context::set(const StringView &name, Value &&val, VarClass *cl) {
	auto it = currentScope->namedVars.emplace(name.str<memory::PoolInterface>()).first;
	it->second.set(move(val), cl);
}

void Context::set(const StringView &name, bool isConst, const Value *val, VarClass *cl) {
	auto it = currentScope->namedVars.emplace(name.str<memory::PoolInterface>()).first;
	it->second.set(isConst, val, cl);
}

void Context::set(const StringView &name, Callback &&cb) {
	auto it = currentScope->namedVars.emplace(name.str<memory::PoolInterface>()).first;
	it->second.set(new Callback(move(cb)));
}

VarClass * Context::set(const StringView &name, VarClass &&cl) {
	auto c_it = classes.emplace(name.str<memory::PoolInterface>(), move(cl)).first;
	auto it = currentScope->namedVars.emplace(name.str<memory::PoolInterface>()).first;
	it->second.set(&c_it->second);
	return &c_it->second;
}

static bool Context_pushMixinVar(Context::Mixin &mixin, Expression *expr) {
	bool requireDefault = (!mixin.args.empty() && mixin.args.back().second);

	if (expr->isToken) {
		if (!requireDefault) {
			++ mixin.required;
			mixin.args.emplace_back(pair(StringView(expr->value.getString()), nullptr));
			return true;
		}
	} else if (expr->op == Expression::Assignment) {
		if (expr->left && expr->left->isToken && expr->right) {
			mixin.args.emplace_back(pair(StringView(expr->left->value.getString()), expr->right));
			return true;
		}
	}
	return false;
};

static bool Context_processMixinArgs(Context::Mixin &mixin, Expression *expr) {
	if (expr) {
		if (expr->isToken || expr->op == Expression::Assignment) {
			return Context_pushMixinVar(mixin, expr);
		} else if (expr->op == Expression::Comma) {
			if (!Context_processMixinArgs(mixin, expr->left)) {
				return false;
			}
			return Context_pushMixinVar(mixin, expr->right);
		} else {
			return false;
		}
	}
	return true;
}

bool Context::setMixin(const StringView &name, const Template::Chunk *chunk) {
	auto it = currentScope->mixins.find(name);
	if (it != currentScope->mixins.end()) {
		return false;
	}

	Mixin mixin{chunk};
	if (chunk->expr && chunk->expr->op == Expression::Call) {
		if (!Context_processMixinArgs(mixin, chunk->expr->right)) {
			return false;
		}
	}

	currentScope->mixins.emplace(name.str<memory::PoolInterface>(), move(mixin));
	return true;
}

const Context::Mixin *Context::getMixin(const StringView &name) const {
	auto scope = currentScope;
	while (scope) {
		auto it = scope->mixins.find(name);
		if (it != scope->mixins.end()) {
			return &it->second;
		}

		scope = scope->prev;
	}
	return nullptr;
}

const VarStorage *Context::getVar(const StringView &name) const {
	ContextFn fn;
	return fn.getVarStorage(currentScope, name);
}

bool Context::runInclude(const StringView &name, std::ostream &out, const Template *tpl) {
	bool ret = false;
	if (_includeCallback) {
		ret = _includeCallback(name, *this, out, tpl);
	}
	if (!ret) {
		out << "<!-- fail to include " << name << " -->";
	}
	return ret;
}

void Context::setIncludeCallback(IncludeCallback &&cb) {
	_includeCallback = move(cb);
}

void Context::pushVarScope(VarScope &scope) {
	scope.prev = currentScope;
	currentScope = &scope;
}

void Context::popVarScope() {
	currentScope = currentScope->prev;
}

static inline bool Context_default_encodeURIChar(char c) {
	if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')
			|| c == '-' || c == '_' || c == '.' || c == '!' || c == '~' || c == '*' || c == '\'' || c == '#'
			|| c == ';' || c == ',' || c == '/' || c == '?' || c == ':' || c == '@' || c == '&' || c == '='
			|| c == '+' || c == '$' || c == '(' || c == ')') {
		return false;
	} else {
		return true;
	}
}

static const String Context_default_encodeURI(const String &data) {
	String ret; ret.reserve(data.size() * 2);
	for (auto &c : data) {
        if (Context_default_encodeURIChar(c)) {
        	ret.push_back('%');
        	ret.append(base16::charToHex(c), 2);
        } else {
        	ret.push_back(c);
        }
	}
	return ret;
}

static inline bool Context_default_encodeURIComponentChar(char c) {
	if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')
			|| c == '-' || c == '_' || c == '.' || c == '!' || c == '~' || c == '*' || c == '\'' || c == '(' || c == ')') {
		return false;
	} else {
		return true;
	}
}

static const String Context_default_encodeURIComponent(const String &data) {
	String ret; ret.reserve(data.size() * 2);
	for (auto &c : data) {
        if (Context_default_encodeURIComponentChar(c)) {
        	ret.push_back('%');
        	ret.append(base16::charToHex(c), 2);
        } else {
        	ret.push_back(c);
        }
	}
	return ret;
}

void Context::loadDefaults() {
	set("encodeURI", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1 && var->readValue().isString()) {
			return pug::Var(data::Value(Context_default_encodeURI(var->readValue().getString())));
		}
		return pug::Var();
	});
	set("encodeURIComponent", [] (pug::VarStorage &, pug::Var *var, size_t argc) -> pug::Var {
		if (var && argc == 1 && var->readValue().isString()) {
			return pug::Var(data::Value(Context_default_encodeURIComponent(var->readValue().getString())));
		}
		return pug::Var();
	});
}

NS_SP_EXT_END(pug)
