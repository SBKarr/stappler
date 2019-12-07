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

#include "ContinueToken.h"

NS_DB_BEGIN

ContinueToken::ContinueToken(const mem::StringView &field, size_t count, bool reverse)
: field(field.str()), count(count), flags(Initial) {
	if (reverse) {
		flags |= Reverse | Inverted;
	}
}

ContinueToken::ContinueToken(const mem::StringView &str) {
	auto bytes = stappler::base64::decode(str);
	auto d = stappler::data::read(bytes);
	if (d.isArray() && d.size() == 6) {
		field = d.getString(0);
		initVec = d.getValue(1);
		count = (size_t)d.getInteger(2);
		fetched = (size_t)d.getInteger(3);
		total = (size_t)d.getInteger(4);
		flags |= Flags(d.getInteger(5));
	}
}

bool ContinueToken::hasPrev() const {
	return hasFlag(Flags::Inverted) ? hasNextImpl() : hasPrevImpl();
}
bool ContinueToken::hasNext() const {
	return hasFlag(Flags::Inverted) ? hasPrevImpl() : hasNextImpl();
}

bool ContinueToken::isInit() const {
	return _init;
}

mem::String ContinueToken::encode() const {
	return stappler::base64url::encode(stappler::data::write(mem::Value({
		mem::Value(field),
		mem::Value(initVec),
		mem::Value(count),
		mem::Value(fetched),
		mem::Value(total),
		mem::Value(stappler::toInt(flags)),
	}), mem::EncodeFormat::Cbor));
}

mem::String ContinueToken::encodeNext() const {
	return hasFlag(Flags::Inverted) ? encodePrevImpl() : encodeNextImpl();
}

mem::String ContinueToken::encodePrev() const {
	return hasFlag(Flags::Inverted) ? encodeNextImpl() : encodePrevImpl();
}

size_t ContinueToken::getStart() const {
	return hasFlag(Flags::Inverted) ? (getTotal() + 1 - fetched - getNumResults()) : (fetched + 1);
}
size_t ContinueToken::getEnd() const {
	return hasFlag(Flags::Inverted) ? std::min(total, getTotal() - fetched) : std::min(total, fetched + getNumResults());
}
size_t ContinueToken::getTotal() const {
	return total;
}
size_t ContinueToken::getCount() const {
	return count;
}
mem::StringView ContinueToken::getField() const {
	return field;
}

size_t ContinueToken::getNumResults() const {
	return _numResults;
}

bool ContinueToken::hasFlag(Flags fl) const {
	return (flags & fl) != Flags::None;
}

void ContinueToken::setFlag(Flags fl) {
	flags |= fl;
}
void ContinueToken::unsetFlag(Flags fl) {
	flags &= (~fl);
}

bool ContinueToken::hasPrevImpl() const {
	return _init && fetched != 0;
}

bool ContinueToken::hasNextImpl() const {
	return _init && fetched + _numResults < total;
}

mem::String ContinueToken::encodeNextImpl() const {
	Flags f = Flags::None;
	if (hasFlag(Flags::Inverted)) { f |= Flags::Inverted; }
	return stappler::base64url::encode(stappler::data::write(mem::Value({
		mem::Value(field),
		mem::Value(lastVec),
		mem::Value(count),
		mem::Value(fetched + _numResults),
		mem::Value(total),
		mem::Value(stappler::toInt(f))
	}), mem::EncodeFormat::Cbor));
}

mem::String ContinueToken::encodePrevImpl() const {
	Flags f = Flags::Reverse;
	if (hasFlag(Flags::Inverted)) { f |= Flags::Inverted; }
	return stappler::base64url::encode(stappler::data::write(mem::Value({
		mem::Value(field),
		mem::Value(firstVec),
		mem::Value(count),
		mem::Value(fetched),
		mem::Value(total),
		mem::Value(stappler::toInt(f))
	}), mem::EncodeFormat::Cbor));
}

mem::Value ContinueToken::perform(const Scheme &scheme, const Transaction &t, Query &q) {
	if (field != "__oid" && !scheme.getField(field)) {
		return mem::Value();
	}

	q.select(field, db::Comparation::IsNotNull, mem::Value(true));
	if (total == 0) {
		total = scheme.count(t, q);
		if (hasFlag(Flags::Reverse) && !fetched) {
			fetched = total;
		}
	}

	if (total == 0) {
		_init = true;
		return mem::Value();
	}

	q.softLimit(field, hasFlag(Flags::Reverse) ? Ordering::Descending : Ordering::Ascending, count, mem::Value(initVec));

	auto d = scheme.select(t, q);
	if (d.isArray()) {
		_numResults = d.size();
		if (hasFlag(Flags::Reverse)) {
			fetched -= (std::min(fetched, _numResults));
			firstVec = d.asArray().back().getValue(field);
			lastVec = d.asArray().front().getValue(field);
		} else {
			firstVec = d.asArray().front().getValue(field);
			lastVec = d.asArray().back().getValue(field);
		}
		_init = true;
	}

	return d;
}

mem::Value ContinueToken::perform(const Scheme &scheme, const Transaction &t, Query &q, Ordering ord) {
	auto rev = hasFlag(Flags::Reverse);
	if (auto d = perform(scheme, t, q)) {
		if ((ord == Ordering::Ascending && rev) || (ord == Ordering::Descending && !rev)) {
			std::reverse(d.asArray().begin(), d.asArray().end());
		}
		return d;
	}
	return mem::Value();
}

mem::Value ContinueToken::performOrdered(const Scheme &scheme, const Transaction &t, Query &q) {
	if (!q.getOrderField().empty()) {
		field = q.getOrderField();
	}

	if (q.getOrdering() == db::Ordering::Descending) {
		setFlag(Flags::Inverted);
	} else {
		unsetFlag(Flags::Inverted);
	}

	return perform(scheme, t, q, q.getOrdering());
}

void ContinueToken::refresh(const Scheme &scheme, const Transaction &t, Query &q) {
	q.select(field, db::Comparation::IsNotNull, mem::Value(true));
	auto newTotal = scheme.count(t, q);

	q.select(field, db::Comparation::LessThen, initVec);
	auto newFetched = scheme.count(t, q);

	total = newTotal;
	fetched = newFetched;
}

NS_DB_END
