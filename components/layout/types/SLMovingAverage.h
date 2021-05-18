/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_TYPES_SLMOVINGAVERAGE_H_
#define LAYOUT_TYPES_SLMOVINGAVERAGE_H_

#include "SPLayout.h"

NS_LAYOUT_BEGIN

template <size_t Count, typename T = float>
class MovingAverage {
public:
	void dropValues() {
		for (size_t i = 0; i < Count; i++) {
			_values[i] = 0;
		}
	}
	void addValue(T value) {
		_values[_current] = value;
		if ((++_current) >= Count) _current = 0;
	}
	T getAverage(bool exceptZero = false) {
		size_t c = 0;
		T s = 0;
		for (size_t i = 0; i < Count; i++) {
			if (!exceptZero || _values[i] != 0) {
				s += _values[i];
				++ c;
			}
		}
		return s / c;
	}
	T step(T value) {
		addValue(value);
		return getAverage();
	}

	T range(bool exceptZero = false) {
		Pair<T, T> minmax(std::numeric_limits<T>::max(), std::numeric_limits<T>::min());

		for (size_t i = 0; i < Count; i++) {
			if (!exceptZero || _values[i] != 0) {
				if (_values[i] < minmax.first) { minmax.first = _values[i]; }
				if (_values[i] > minmax.second) { minmax.second = _values[i]; }
			}
		}

		if (minmax.first <= minmax.second) {
			return minmax.second - minmax.first;
		} else {
			return 0;
		}
	}

	size_t size() const { return Count; }

protected:
	size_t _current = 0;
    std::array<T, Count> _values;
};

NS_LAYOUT_END

#endif
