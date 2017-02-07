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

#ifndef STAPPLER_LAYOUT_STYLE_SPLAYOUTSTYLEPARAMETERS_HPP_
#define STAPPLER_LAYOUT_STYLE_SPLAYOUTSTYLEPARAMETERS_HPP_

#include "SLStyle.h"

NS_LAYOUT_BEGIN

namespace style {

template<ParameterName Name, class Value> Parameter Parameter::create(const Value &v, MediaQueryId query) {
	Parameter p(Name, query);
	p.set<Name>(v);
	return p;
}

template<ParameterName Name, class Value> void ParameterList::set(const Value &value, MediaQueryId mediaQuery) {
	if (mediaQuery != MediaQueryNone() && !isAllowedForMediaQuery(Name)) {
		return;
	}
	for (auto &it : data) {
		if (it.name == Name && it.mediaQuery == mediaQuery) {
			it.set<Name>(value);
			return;
		}
	}

	data.push_back(Parameter::create<Name>(value, mediaQuery));
}

}

NS_LAYOUT_END

#endif /* STAPPLER_LAYOUT_STYLE_SPLAYOUTSTYLEPARAMETERS_HPP_ */
