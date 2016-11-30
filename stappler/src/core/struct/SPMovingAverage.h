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

#ifndef chieftime_federal_SPMovingAverage_h
#define chieftime_federal_SPMovingAverage_h

#include "SPDefine.h"

NS_SP_BEGIN

class MovingAverage {
protected:
    int _count, _current;
    float *_values;
public:
    MovingAverage() : _count(0), _current(0) {
        _values = nullptr;
    }
    MovingAverage(int count) : _count(count), _current(0) {
        _values = new float[_count];
        memset(_values, 0, sizeof(float) * _count);
    }
    ~MovingAverage() {
        if (_values) {
            delete [] _values;
			_values = nullptr;
        }
    }
    
    void init(int count) {
        if (_values) {
            delete [] _values;
            _values = NULL;
        }
        _count = count;
        _values = new float[_count];
        memset(_values, 0, sizeof(float) * _count);
    }
    
    void dropValues() {
        for (int i = 0; i < _count; i++) {
            _values[i] = 0;
        }
    }
    void addValue(float value) {
        _values[_current] = value;
        if ((++_current) >= _count) _current = 0;
    }
    float getAverage() {
        float s = 0;
        for (int i = 0; i < _count; i++) {
            s += _values[i];
        }
        return s / _count;
    }
    float step(float value) {
        addValue(value);
        return getAverage();
    }
};

NS_SP_END

#endif
