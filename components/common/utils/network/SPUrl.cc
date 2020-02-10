// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPString.h"
#include "SPUrl.h"
#include "SPStringView.h"
#include "SPUrlencodeParser.h"
#include "SPSearchParser.h"

NS_SP_BEGIN

Vector<String> UrlView::parsePath(const StringView &str) {
	Vector<String> ret;
	StringView s(str);
	do {
		if (s.is('/')) {
			s ++;
		}
		auto path = s.readUntil<StringView::Chars<'/', '?', ';', '&', '#'>>();
		if (path == "..") {
			if (!ret.empty()) {
				ret.pop_back();
			}
		} else if (path == ".") {
			// skip this component
		} else {
			if (!path.empty()) {
				ret.push_back(path.str());
			}
		}
	} while (!s.empty() && s.is('/'));
	return ret;
}

bool UrlView::isValidIdnTld(StringView str) {
	if (
// Generic IDN TLD
			str == "موقع"
			|| str == "كوم"
			|| str == "كوم"
			|| str == "موبايلي"
			|| str == "كاثوليك"
			|| str == "شبكة"
			|| str == "بيتك"
			|| str == "بازار"
			|| str == "在线"
			|| str == "中文网"
			|| str == "网址"
			|| str == "网站"
			|| str == "网络"
			|| str == "公司"
			|| str == "商城"
			|| str == "机构"
			|| str == "我爱你"
			|| str == "商标"
			|| str == "世界"
			|| str == "集团"
			|| str == "慈善"
			|| str == "公益"
			|| str == "八卦"
			|| str == "дети"
			|| str == "католик"
			|| str == "ком"
			|| str == "онлайн"
			|| str == "орг"
			|| str == "сайт"
			|| str == "संगठन"
			|| str == "कॉम"
			|| str == "नेट"
			|| str == "닷컴"
			|| str == "닷넷"
			|| str == "קום"
			|| str == "みんな"
			|| str == "セール"
			|| str == "ファッション"
			|| str == "ストア"
			|| str == "ポイント"
			|| str == "クラウド"
			|| str == "コム"
			|| str == "คอม"

// Country-code IDN TLD
			|| str == "الجزائر"
			|| str == "հայ"
			|| str == "البحرين"
			|| str == "বাংলা"
			|| str == "бел"
			|| str == "бг"
			|| str == "中国"
			|| str == "中國"
			|| str == "مصر"
			|| str == "ею"
			|| str == "ευ"
			|| str == "გე"
			|| str == "ελ"
			|| str == "香港"
			|| str == "भारत"
			|| str == "بھارت"
			|| str == "భారత్"
			|| str == "ભારત"
			|| str == "ਭਾਰਤ"
			|| str == "இந்தியா"
			|| str == "ভারত"
			|| str == "ಭಾರತ"
			|| str == "ഭാരതം"
			|| str == "ভাৰত"
			|| str == "ଭାରତ "
			|| str == "بارت"
			|| str == "भारतम्"
			|| str == "भारोत"
			|| str == "ڀارت"
			|| str == "ایران"
			|| str == "عراق"
			|| str == "الاردن"
			|| str == "қаз"
			|| str == "ລາວ"
			|| str == "澳门"
			|| str == "澳門"
			|| str == "مليسيا"
			|| str == "موريتانيا"
			|| str == "мон"
			|| str == "المغرب"
			|| str == "мкд"
			|| str == "عمان"
			|| str == "پاکستان"
			|| str == "فلسطين"
			|| str == "قطر"
			|| str == "рф"
			|| str == "السعودية"
			|| str == "срб"
			|| str == "新加坡"
			|| str == "சிங்கப்பூர்"
			|| str == "한국"
			|| str == "ලංකා"
			|| str == "இலங்கை"
			|| str == "سودان"
			|| str == "سورية"
			|| str == "台湾"
			|| str == "台灣"
			|| str == "ไทย"
			|| str == "تونس"
			|| str == "укр"
			|| str == "امارات"
			|| str == "اليمن"

// Internationalized geographic top-level domains
			|| str == "佛山"
			|| str == "广东"
			|| str == "москва"
			|| str == "рус"
			|| str == "ابوظبي"
			|| str == "عرب"

// Internationalized brand top-level domains
			|| str == "ارامكو"
			|| str == "اتصالات"
			|| str == "联通"
			|| str == "移动"
			|| str == "中信"
			|| str == "香格里拉"
			|| str == "淡马锡"
			|| str == "大众汽车"
			|| str == "vermögensberater"
			|| str == "vermögensberatung"
			|| str == "グーグル"
			|| str == "谷歌"
			|| str == "gǔgē"
			|| str == "工行"
			|| str == "嘉里"
			|| str == "嘉里大酒店"
			|| str == "飞利浦"
			|| str == "诺基亚"
			|| str == "電訊盈科"
			|| str == "삼성"
		) {
			return true;
	}
	return false;
}

data::Value UrlView::parseArgs(const StringView &str, size_t maxVarSize) {
	if (str.empty()) {
		return data::Value();
	}
	data::Value ret;
	StringView r(str);
	if (r.front() == '?' || r.front() == '&' || r.front() == ';') {
		++ r;
	}
	UrlencodeParser parser(ret, r.size(), maxVarSize);
	parser.read((const uint8_t *)r.data(), r.size());
	return ret;
}

UrlView::UrlView() { }

UrlView::UrlView(const StringView &str) {
	parse(str);
}

void UrlView::clear() {
	scheme.clear();
	user.clear();
	password.clear();
	host.clear();
	port.clear();
	path.clear();
	query.clear();
	fragment.clear();
	url.clear();
}

bool UrlView::parse(const StringView &str) {
	StringView r(str);
	return parse(r);
}

bool UrlView::parse(StringView &str) {
	auto tmp = str;
	if (search::parseUrl(str, [&] (StringView str, search::UrlToken tok) {
		switch (tok) {
		case search::UrlToken::Scheme: scheme = str; break;
		case search::UrlToken::User: user = str; break;
		case search::UrlToken::Password: password = str; break;
		case search::UrlToken::Host: host = str; break;
		case search::UrlToken::Port: port = str; break;
		case search::UrlToken::Path: path = str; break;
		case search::UrlToken::Query: query = str; break;
		case search::UrlToken::Fragment: fragment = str; break;
		case search::UrlToken::Blank: break;
		}
	})) {
		url = StringView(tmp.data(), str.data() - tmp.data());
		return true;
	} else {
		clear();
		return false;
	}
}

static void UrlView_get(std::ostream &stream, const UrlView &view) {
	if (!view.scheme.empty()) {
		stream << view.scheme << ":";
	}
	if (!view.scheme.empty() && !view.host.empty() && view.scheme != "mailto") {
		stream << "//";
	}
	if (!view.host.empty()) {
		if (!view.user.empty()) {
			stream << view.user;
			if (!view.password.empty()) {
				stream << ":" << view.password;
			}
			stream << "@";
		}
		stream << view.host;
		if (!view.port.empty()) {
			stream << ":" << view.port;
		}
	}
	if (!view.path.empty()) {
		stream << view.path;
	}
	if (!view.query.empty()) {
		stream << "?" << view.query;
	}
	if (!view.fragment.empty()) {
		stream << "#" << view.fragment;
	}
}

SP_TEMPLATE_MARK
auto UrlView::get<memory::PoolInterface>() const -> memory::PoolInterface::StringType {
	memory::PoolInterface::StringStreamType stream;
	UrlView_get(stream, *this);
	return stream.str();
}

SP_TEMPLATE_MARK
auto UrlView::get<memory::StandartInterface>() const -> memory::StandartInterface::StringType {
	memory::StandartInterface::StringStreamType stream;
	UrlView_get(stream, *this);
	return stream.str();
}

bool UrlView::isEmail() const {
	return (!user.empty() && !host.empty()) && (scheme.empty() && password.empty() && port.empty() && path.empty() && query.empty() && fragment.empty());
}

bool UrlView::isPath() const {
	return !path.empty() && (scheme.empty() && user.empty() && password.empty() && host.empty() && port.empty() && query.empty() && fragment.empty());
}

NS_SP_END
