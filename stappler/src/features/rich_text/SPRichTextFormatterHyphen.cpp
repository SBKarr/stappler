#include "SPDefine.h"
#include "SPRichTextFormatter.h"
#include "SPFilesystem.h"

NS_SP_EXT_BEGIN(rich_text)

bool HyphenMap::init() {
	return true;
}

void HyphenMap::addHyphenDict(CharGroupId id, const String &file) {
	auto data = filesystem::readTextFile(file);
	if (!data.empty()) {
		auto dict = hnj_hyphen_load_data(data.data(), data.size());
		if (dict) {
			auto it = _dicts.find(id);
			if (it == _dicts.end()) {
				_dicts.emplace(id, dict);
			} else {
				hnj_hyphen_free(it->second);
				it->second = dict;
			}
		}
	}
}
Vector<uint8_t> HyphenMap::makeWordHyphens(const char16_t *ptr, size_t len) {
	if (len < 4 || len >= 255) {
		return Vector<uint8_t>();
	}

	HyphenDict *dict = nullptr;
	for (auto &it : _dicts) {
		if (inCharGroup(it.first, ptr[0])) {
			dict = it.second;
			break;
		}
	}

	if (!dict) {
		return Vector<uint8_t>();
	}

	String word = convertWord(dict, ptr, len);
	if (!word.empty()) {
		Vector<char> buf; buf.resize(word.size() + 5);

		char ** rep = nullptr;
		int * pos = nullptr;
		int * cut = nullptr;
		hnj_hyphen_hyphenate2(dict, word.data(), word.size(), buf.data(), nullptr, &rep, &pos, &cut);

		Vector<uint8_t> ret;
		uint8_t i = 0;
		for (auto &it : buf) {
			if (it > 0) {
				if ((it - '0') % 2 == 1) {
					ret.push_back(i + 1);
				}
			} else {
				break;
			}
			++ i;
		}
		return ret;
	}
	return Vector<uint8_t>();
}
Vector<uint8_t> HyphenMap::makeWordHyphens(const CharReaderUcs2 &r) {
	return makeWordHyphens(r.data(), r.size());
}
void HyphenMap::purgeHyphenDicts() {
	for (auto &it : _dicts) {
		hnj_hyphen_free(it.second);
	}
}

String HyphenMap::convertWord(HyphenDict *dict, const char16_t *ptr, size_t len) {
	if (dict->utf8) {
		return string::toUtf8(ptr, len);
	} else {
		if (strcmp("KOI8-R", dict->cset) == 0) {
			return string::toKoi8r(ptr, len);
		}

		return String();
	}
}

NS_SP_EXT_END(rich_text)
