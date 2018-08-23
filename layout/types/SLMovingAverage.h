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

template <size_t Count>
class MovingAverage {
public:
    void dropValues() {
        for (int i = 0; i < Count; i++) {
            _values[i] = 0;
        }
    }
    void addValue(float value) {
        _values[_current] = value;
        if ((++_current) >= Count) _current = 0;
    }
    float getAverage() {
        float s = 0;
        for (size_t i = 0; i < Count; i++) {
            s += _values[i];
        }
        return s / Count;
    }
    float step(float value) {
        addValue(value);
        return getAverage();
    }

protected:
	size_t _current = 0;
    std::array<float, Count> _values;
};

NS_LAYOUT_END

#endif
