/*
 * SPRichTextStyleParameters.hpp
 *
 *  Created on: 29 июля 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTSTYLEPARAMETERS_HPP_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTSTYLEPARAMETERS_HPP_

#include "SPRichTextStyle.h"

NS_SP_EXT_BEGIN(rich_text)

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

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_DOCUMENT_SPRICHTEXTSTYLEPARAMETERS_HPP_ */
