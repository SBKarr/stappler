// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPCommon.h"
#include "SPSearchDistance.h"

NS_SP_EXT_BEGIN(search)

Distance::Storage Distance::Storage::merge(const Storage &first, const Storage &last) {
	Distance::Storage ret(first);
	ret.reserve(first.size() + last.size());
	for (size_t i = 0; i < last.size(); ++ i) {
		ret.emplace_back(last.at(i));
	}

	return ret;
}

Distance::Storage::Storage() noexcept : _size({0, 0}) {
	new (&_array) Array();
}
Distance::Storage::~Storage() noexcept {
	if (isVecStorage()) {
		_bytes.~vector();
	} else {
		_array.~array();
	}
}

Distance::Storage::Storage(const Storage &other) noexcept {
	_size = other._size;
	if (other.isVecStorage()) {
		new (&_bytes) Vec(other._bytes);
	} else {
		new (&_array) Array(other._array);
	}
}
Distance::Storage::Storage(Storage &&other) noexcept {
	_size = other._size;
	if (other.isVecStorage()) {
		new (&_bytes) Vec(move(other._bytes));
	} else {
		new (&_array) Array(other._array);
	}
	other.clear();
}

Distance::Storage &Distance::Storage::operator=(const Storage &other) noexcept {
	clear();
	if (other.isVecStorage()) {
		_array.~array();
		new (&_bytes) Vec(other._bytes);
	} else {
		_array = other._array;
	}
	_size = other._size;
	return *this;
}

Distance::Storage &Distance::Storage::operator=(Storage &&other) noexcept {
	clear();
	if (other.isVecStorage()) {
		_array.~array();
		new (&_bytes) Vec(move(other._bytes));
	} else {
		_array = other._array;
	}
	_size = other._size;
	other.clear();
	return *this;
}

bool Distance::Storage::empty() const {
	return _size.size == 0;
}

size_t Distance::Storage::size() const {
	return _size.size;
}

size_t Distance::Storage::capacity() const {
	if (isVecStorage()) {
		return _bytes.size() * 4;
	} else {
		return _array.size() * 4;
	}
}

void Distance::Storage::reserve(size_t s) {
	if (isVecStorage() && isVecStorage(s)) {
		_bytes.resize((s + 3) / 4);
	} else if (isVecStorage(s)) {
		auto tmp = _array;
		_array.~array();
		new (&_bytes) Vec();
		_bytes.resize((s + 3) / 4);
		memcpy(_bytes.data(), tmp.data(), tmp.size());
		_size.vec = 1;
	}
}

void Distance::Storage::emplace_back(Value val) {
	(isVecStorage() ? _bytes.data() : _array.data())[_size.size  / 4].set(_size.size  % 4, val);
	++ _size.size;
}

void Distance::Storage::reverse() {
	for (size_t i = 0; i < _size.size / 2; ++ i) {
		auto b = at(_size.size - i - 1);
		set(_size.size - i - 1, at(i));
		set(i, b);
	}
}

Distance::Value Distance::Storage::at(size_t idx) const {
	return (isVecStorage() ? _bytes.data() : _array.data())[idx / 4].get(idx % 4);
}

void Distance::Storage::set(size_t idx, Value val) {
	(isVecStorage() ? _bytes.data() : _array.data())[idx / 4].set(idx % 4, val);
}

void Distance::Storage::clear() {
	if (isVecStorage()) {
		_bytes.~vector();
		new (&_array) Array();
		_size.vec = 0;
	} else {
		memset(_array.data(), 0, _array.size());
	}
	_size.size = 0;
}

bool Distance::Storage::isVecStorage() const {
	return _size.vec;
}

bool Distance::Storage::isVecStorage(size_t s) const {
	return (s + 3) / 4 > sizeof(Bytes);
}


Distance::Distance() noexcept { }

Distance::Distance(const Distance &dist) noexcept : _storage(dist._storage) { }
Distance::Distance(Distance &&dist) noexcept : _storage(move(dist._storage)) { }

Distance &Distance::operator=(const Distance &dist) noexcept {
	_storage = dist._storage;
	return *this;
}
Distance &Distance::operator=(Distance &&dist) noexcept {
	_storage = move(dist._storage);
	return *this;
}

bool Distance::empty() const {
	return _storage.empty();
}

size_t Distance::size() const {
	return _storage.size();
}

int32_t Distance::diff_original(size_t pos, bool forward) const {
	if (empty()) {
		return pos;
	}

	int32_t ret = 0;
	size_t i = 0;
	pos = min(pos, size());
	for (; i < pos; ++ i) {
		switch (_storage.at(i)) {
		case Value::Match:
		case Value::Replace:
			// do nothing
			break;
		case Value::Delete:
			++ ret;
			break;
		case Value::Insert:
			-- ret;
			break;
		}
	}
	if (forward) {
		while (i < size() && _storage.at(i) == Value::Delete) {
			++ ret;
			++ i;
		}
	}
	return ret;
}

int32_t Distance::diff_canonical(size_t pos, bool forward) const {
	if (empty()) {
		return pos;
	}

	int32_t ret = 0;
	size_t i = 0;
	pos = min(pos, size());
	for (; i < pos; ++ i) {
		switch (_storage.at(i)) {
		case Value::Match:
		case Value::Replace:
			// do nothing
			break;
		case Value::Delete:
			-- ret;
			break;
		case Value::Insert:
			++ ret;
			break;
		}
	}
	if (forward) {
		while (i < size() && _storage.at(i) == Value::Insert) {
			++ ret;
			++ i;
		}
	}
	return ret;
}

memory::string Distance::info() const {
	const auto s = _storage.size();
	memory::string ret; ret.reserve(s);

    char moveCodeToChar[] = {'=', '+', '-', 'X'};

    char lastMove = 0;  // Char of last move. 0 if there was no previous move.
    int numOfSameMoves = 0;
    for (size_t i = 0; i <= s; i++) {
        // if new sequence of same moves started
        if (i == s || (moveCodeToChar[toInt(_storage.at(i))] != lastMove && lastMove != 0)) {
            // Write number of moves to cigar string.
            int numDigits = 0;
            for (; numOfSameMoves; numOfSameMoves /= 10) {
            	ret.push_back('0' + numOfSameMoves % 10);
                numDigits++;
            }
            std::reverse(ret.end() - numDigits, ret.end());
            // Write code of move to cigar string.
            ret.push_back(lastMove);
            // If not at the end, start new sequence of moves.
            if (i < s) {
                // Check if alignment has valid values.
                if (toInt(_storage.at(i)) > 3) {
                    return memory::string();
                }
                numOfSameMoves = 0;
            }
        }
        if (i < s) {
            lastMove = moveCodeToChar[toInt(_storage.at(i))];
            numOfSameMoves++;
        }
    }

	return ret;
}

NS_SP_EXT_END(search)
