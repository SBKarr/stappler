//
//  SPMovingAverage.h
//  chieftime-federal
//
//  Created by SBKarr on 26.06.13.
//
//

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
