/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "ResourceTemplates.h"
#include "SPCharGroup.h"

NS_SA_BEGIN

ResourceSearch::ResourceSearch(const Adapter &a, QueryList &&q, const Field *prop)
: ResourceObject(a, move(q)), _field(prop) {
	_type = ResourceType::Search;
}

data::Value ResourceSearch::getResultObject() {
	auto slot = _field->getSlot<db::FieldFullTextView>();
	if (auto &searchData = _queries.getExtraData().getValue("search")) {
		Vector<db::FullTextData> q = slot->parseQuery(searchData);
		stappler::search::Language lang = stappler::search::Language::English;
		for (auto &it : q) {
			if (it.language != lang) {
				lang = it.language;
			}
			it.language = search::Language::Simple; // prevent to secondary stem
		}
		_config.setLanguage(lang);

		if (!q.empty()) {
			_queries.setFullTextQuery(_field, Vector<db::FullTextData>(q));
			auto ret = _transaction.performQueryList(_queries);
			if (!ret.isArray()) {
				return data::Value();
			}

			auto res = processResultList(_queries, ret);
			if (!res.empty()) {
				if (auto &headlines = _queries.getExtraData().getValue("headlines")) {
					auto ql = _config.stemQuery(q);
					for (auto &it : res.asArray()) {
						makeHeadlines(it, headlines, ql);
					}
				}
			}
			return res;
		}
	}
	return data::Value();
}

/* Vector<String> ResourceSearch::stemQuery(const Vector<db::FullTextData> &query) {
	Vector<String> ret; ret.reserve(256 / sizeof(String)); // memory manager hack

	for (auto &it : query) {
		StringView r(it.buffer);
		r.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (StringView &iword) {
			StringViewUtf8 word(iword.data(), iword.size());
			word.trimUntil<StringViewUtf8::MatchCharGroup<CharGroupId::Cyrillic>, StringViewUtf8::MatchCharGroup<CharGroupId::Alphanumeric>>();
			if (word.size() > 3) {
				ret.emplace_back(_stemmer.stemWord(word, it.language).str());
			}
		});
	}

	return ret;
} */

void ResourceSearch::makeHeadlines(data::Value &obj, const data::Value &headlineInfo, const Vector<String> &ql) {
	auto &h = obj.emplace("__headlines");
	for (auto &it : headlineInfo.asDict()) {
		auto d = getObjectLine(obj, it.first);
		if (d && d->isString()) {
			auto headStr = makeHeadline(d->getString(), it.second, ql);
			if (!headStr.empty()) {
				h.setString(headStr, it.first);
			}
		}
	}
}

String ResourceSearch::makeHeadline(const StringView &value, const data::Value &headlineInfo, const Vector<String> &ql) {
	stappler::search::Configuration::HeadlineConfig cfg;
	if (headlineInfo.isString()) {
		if (headlineInfo.getString() == "plain") {
			cfg.startToken = StringView("<b>"); cfg.stopToken = StringView("</b>");
			return _config.makeHeadline(cfg, value, ql);
		} else if (headlineInfo.getString() == "html") {
			cfg.startToken = StringView("<b>"); cfg.stopToken = StringView("</b>");
			cfg.startFragment = StringView("<p>"); cfg.stopFragment = StringView("</p>");
			return _config.makeHtmlHeadlines(cfg, value, ql);
		}
	} else if (headlineInfo.isDictionary()) {
		auto type = headlineInfo.getString("type");
		auto start = headlineInfo.getString("start");
		auto end = headlineInfo.getString("end");

		auto l = search::parseLanguage(headlineInfo.getString("lang"));
		if (l != search::Language::Unknown && l != _config.getLanguage()) {
			_config.setLanguage(l);
		}
		if (type == "html") {
			if (!start.empty() && start.size() < 24 && !end.empty() && end.size() < 24) {
				cfg.startToken = start; cfg.stopToken = end;
			} else {
				cfg.startToken = StringView("<b>"); cfg.stopToken = StringView("</b>");
			}

			auto fragStart = headlineInfo.getString("fragStart");
			auto fragEnd = headlineInfo.getString("fragStop");
			if (!fragStart.empty() && fragStart.size() < 24 && !fragEnd.empty() && fragEnd.size() < 24) {
				cfg.startFragment = fragStart; cfg.stopFragment = fragEnd;
			} else {
				cfg.startFragment = StringView("<p>"); cfg.stopFragment = StringView("</p>");
			}

			cfg.maxWords = headlineInfo.getInteger("maxWords", search::Configuration::HeadlineConfig::DefaultMaxWords);
			cfg.minWords = headlineInfo.getInteger("minWords", search::Configuration::HeadlineConfig::DefaultMinWords);
			cfg.shortWord = headlineInfo.getInteger("shortWord", search::Configuration::HeadlineConfig::DefaultShortWord);

			size_t frags = 1;
			auto fragments = headlineInfo.getValue("fragments");
			if (fragments.isString() && fragments.getString() == "max") {
				frags = maxOf<size_t>();
			} else if (auto f = fragments.asInteger()) {
				frags = f;
			}

			return _config.makeHtmlHeadlines(cfg, value, ql, frags);
		} else {
			if (!start.empty() && start.size() < 24 && !end.empty() && end.size() < 24) {
				cfg.startToken = start; cfg.stopToken = end;
			} else {
				cfg.startToken = StringView("<b>"); cfg.stopToken = StringView("</b>");
			}

			return _config.makeHeadline(cfg, value, ql);
		}
	}
	return String();
}

const data::Value *ResourceSearch::getObjectLine(const data::Value &obj, const StringView &key) {
	const data::Value *ptr = &obj;
	key.split<StringView::Chars<'.'>>([&] (const StringView &k) {
		if (ptr && ptr->isDictionary()) {
			if (auto &v = ptr->getValue(k)) {
				ptr = &v;
			}
		} else {
			ptr = nullptr;
		}
	});
	return ptr;
}

NS_SA_END
