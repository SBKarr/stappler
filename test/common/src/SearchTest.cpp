// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPData.h"
#include "Test.h"

#include "SPSearchConfiguration.h"
#include "SPUrl.h"

NS_SP_BEGIN

struct SearchTest : MemPoolTest {
	SearchTest() : MemPoolTest("SearchTest") { }

	virtual bool run(pool_t *pool) {
		StringStream stream;
		size_t count = 0;
		size_t passed = 0;
		stream << "\n";

		runTest(stream, "StringViewUtf8 reverse", count, passed, [&] {
			String test1("——test——");
			String test2("test——");
			String test3("тест——тест");
			String test4("——тест");

			StringViewUtf8 r1(test1);
			r1.trimUntil<StringViewUtf8::CharGroup<CharGroupId::Latin>>();

			StringViewUtf8 r2(test2);
			auto r3 = r2.backwardReadUntil<StringViewUtf8::CharGroup<CharGroupId::Latin>>();

			StringViewUtf8 r4(test3);
			r4.trimChars<StringViewUtf8::CharGroup<CharGroupId::Cyrillic>>();

			StringViewUtf8 r5(test4);
			auto r6 = r5.backwardReadChars<StringViewUtf8::CharGroup<CharGroupId::Cyrillic>>();

			if (r1 == "test" && r2 == "test" && r3 == "——" && r4 == "——" && r5 == "——" && r6 == "тест") {
				return true;
			} else {
				stream << "'" << r1 << "':" << (r1 == "test")
						<< " '" << r2 << "':" << (r2 == "test")
						<< " '" << r3 << "':" << (r3 == "——")
						<< " '" << r4 << "':" << (r4 == "——")
						<< " '" << r5 << "':" << (r5 == "——")
						<< " '" << r6 << "':" << (r6 == "тест");
				return false;
			}
		});

		runTest(stream, "StringView table optimization", count, passed, [&] {
			StringView test1("test \n\t test");
			test1.skipUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
			auto r2 = test1.readChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

			StringView test2("tEst12TeST test");
			auto r3 = test2.readChars<StringView::CharGroup<CharGroupId::Alphanumeric>>();

			return r2 == " \n\t " && r3 == "tEst12TeST" && test2 == " test";
		});

		runTest(stream, "Search configuration", count, passed, [&] {
			search::Configuration cfg(search::Language::Russian);

			String str = "кад. №31:26:00 00 :14239/0/11/04:1001/Б условный номер: 36–34–6:00–00–00:00:2780:2–27–3. Кадастровый №52:18:05 00 00:0000:05965:II."
					"Нижний Новгород, ул. Правдинская, д.27. Ограничение (обременение) права: ипотека."
					"3 км Юго-Запад от х. Новая Паника. 	 	86 611,00	34:34:110011:322  50-50-49 008 2012-057Вто­рой и 50-50-49 008 2012-057 тре­тий эта­пы этой мо­де­ли когда-то - сбор и об­ра­ботка ин­фор­ма­ции - ха­рак­те­ри­зу­ют способ­ность "
					"че­ло­ве­ка быть ра­ци­о­наль­ным. Ра­ци­о­наль­ность - это способ­ность че­ло­ве­ка при­ни­мать пра­виль­ные ре­ше­ния "
					"в де­лу ин­фор­ма­цию, а так­же при­нять ре­ше­ние (по­дроб­но эти во­про­сы изу­ча­ют в раз­де­ле «По­тре­би­тель­ский вы­бор» эко­но­ми­че­ской тео­рии).";

			cfg.stemPhrase(str, [&] (StringView word, StringView stem, search::ParserToken tok) {
				std::cout << word << " -> " << stem << " (" << search::getParserTokenName(tok) << ")\n";
			});

			search::Configuration::HeadlineConfig hcfg;
			hcfg.startToken = StringView("<b>");
			hcfg.stopToken = StringView("</b>");

			std::cout << cfg.makeHeadline(hcfg, str, Vector<String>{ "когда-т", "модел", "сбор" }) << "\n";
			std::cout << cfg.makeHeadline(hcfg, str, Vector<String>{ "модел", "сбор" }) << "\n";
			std::cout << cfg.makeHeadline(hcfg, str, Vector<String>{ "втор", "трет" }) << "\n";

			return true;
		});

		runTest(stream, "Search parser offset", count, passed, [&] {
			//auto str = StringView("up-to-date postgresql-beta1 123test  123.test -1.234e56 -1.234 -1234 1234 8.3.0 &amp;");
			auto str = StringView("-1.234e56 -1.234 -1234 1234 8.3.0 &amp; a_nichkov@mail.ru");

			Vector<Pair<StringView, search::ParserToken>> vec{
				pair("-1.234e56", search::ParserToken::ScientificFloat),
				pair("-1.234", search::ParserToken::Float),
				pair("-1234", search::ParserToken::Integer),
				pair("1234", search::ParserToken::Integer),
				pair("8.3.0", search::ParserToken::Version),
				pair("&amp;", search::ParserToken::XMLEntity),
				pair("a_nichkov@mail.ru", search::ParserToken::Email),
			};

			bool success = true;
			size_t idx = 0;
			search::parsePhrase(str, [&] (StringView str, search::ParserToken tok) {
				switch (tok) {
				case search::ParserToken::Blank: break;
				default:
					if (vec[idx].first != str || vec[idx].second != tok) {
						success = false;
					}
					++ idx;
					break;
				}
				return search::ParserStatus::Continue;
			});
			return success;
		});

		runTest(stream, "Search parser", count, passed, [&] {
			//auto str = StringView("up-to-date postgresql-beta1 123test  123.test -1.234e56 -1.234 -1234 1234 8.3.0 &amp;");
			auto str = StringView("31:26:00 00 :14239/0/11/04:1001/Б 36–34–6:00–00–00:00:2780:2–27–3 №52:18:05 00 00:0000:05965 123test "
					"77-77-09/020/2008-082  63:01:0736001:550  63:01:0736001:550 "
					"/usr/local/foo.txt test/stest 12.34.56.78:1234 12.34.56.78: "
					"12.34.56.78@1234 12.34.56.78@test 12@test 12.34@test "
					"John.Doe@example.com mailto:John.Doe@example.com up-to-date https://sbkarr@127.0.0.1:1234 "
					"postgresql-beta1 123.test -1.234e56 -1.234 -1234 1234 8.3.0 &amp; a_nichkov@mail.ru");

			Vector<Pair<StringView, search::ParserToken>> vec{
				pair("31:26:00 00 :14239", search::ParserToken::Custom),
				pair("/0/11/04:1001/Б", search::ParserToken::Path),
				pair("36–34–6:00–00–00:00:2780:2–27–3", search::ParserToken::Custom),
				pair("52:18:05 00 00:0000:05965", search::ParserToken::Custom),
				pair("123test", search::ParserToken::NumWord),
				pair("77-77-09/020/2008-082", search::ParserToken::Custom),
				pair("63:01:0736001:550", search::ParserToken::Custom),
				pair("63:01:0736001:550", search::ParserToken::Custom),
				pair("/usr/local/foo.txt", search::ParserToken::Path),
				pair("test/stest", search::ParserToken::Url),
				pair("12.34.56.78:1234", search::ParserToken::Url),
				pair("12.34.56.78", search::ParserToken::Version),
				pair("12.34.56.78@1234", search::ParserToken::Email),
				pair("12.34.56.78@test", search::ParserToken::Email),
				pair("12@test", search::ParserToken::Email),
				pair("12.34@test", search::ParserToken::Email),
				pair("John.Doe@example.com", search::ParserToken::Email),
				pair("mailto:John.Doe@example.com", search::ParserToken::Url),
				pair("up-to-date", search::ParserToken::AsciiHyphenatedWord),
				pair("up", search::ParserToken::HyphenatedWord_AsciiPart),
				pair("to", search::ParserToken::HyphenatedWord_AsciiPart),
				pair("date", search::ParserToken::HyphenatedWord_AsciiPart),
				pair("https://sbkarr@127.0.0.1:1234", search::ParserToken::Url),
				pair("postgresql-beta1", search::ParserToken::NumHyphenatedWord),
				pair("postgresql", search::ParserToken::HyphenatedWord_AsciiPart),
				pair("beta1", search::ParserToken::HyphenatedWord_NumPart),
				pair("123", search::ParserToken::Integer),
				pair("test", search::ParserToken::AsciiWord),
				pair("-1.234e56", search::ParserToken::ScientificFloat),
				pair("-1.234", search::ParserToken::Float),
				pair("-1234", search::ParserToken::Integer),
				pair("1234", search::ParserToken::Integer),
				pair("8.3.0", search::ParserToken::Version),
				pair("&amp;", search::ParserToken::XMLEntity),
				pair("a_nichkov@mail.ru", search::ParserToken::Email),
			};

			bool success = true;
			size_t idx = 0;
			search::parsePhrase(str, [&] (StringView str, search::ParserToken tok) {
				switch (tok) {
				case search::ParserToken::Blank: break;
				default:
					if (vec[idx].first != str || vec[idx].second != tok) {
						success = false;
					}
					++ idx;
					break;
				}
				return search::ParserStatus::Continue;
			});
			return success;
		});

		runTest(stream, "Search url parser", count, passed, [&] {
			Vector<String> urls;
			urls.emplace_back("https://йакреведко.рф/test/.././..////?query[креведко][treas][ds][]=qwert#аяклешня");
			urls.emplace_back("mailto:user@example.org?subject=caf%C3%A9&body=caf%C3%A9");
			urls.emplace_back("mailto:John.Doe@example.com");
			urls.emplace_back("https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top");
			urls.emplace_back("ldap://[2001:db8::7]/c=GB?objectClass?one");
			urls.emplace_back("news:comp.infosystems.www.servers.unix");
			urls.emplace_back("tel:+1-816-555-1212");
			urls.emplace_back("telnet://192.0.2.16:80/");
			urls.emplace_back("urn:oasis:names:specification:docbook:dtd:xml:4.1.2");
			urls.emplace_back("user@[2001:db8::1]");
			urls.emplace_back("user:123@[2001:db8::1]:1234");
			urls.emplace_back("[2001:db8::1]/test");
			urls.emplace_back("sbkarr@127.0.0.1:1234");
			urls.emplace_back("https://sbkarr@127.0.0.1:1234");
			urls.emplace_back("git:1234@github.com:SBKarr/stappler.git");
			urls.emplace_back("doi:10.1000/182");
			urls.emplace_back("git@github.com:SBKarr/stappler.git");
			urls.emplace_back("ssh://git@atom0.stappler.org:21002/StapplerApi.git?qwr#test");
			urls.emplace_back("localhost");
			urls.emplace_back("localhost:8080");
			urls.emplace_back("localhost/test1/test2");
			urls.emplace_back("/usr/local/bin/bljad");
			urls.emplace_back("https://localhost/test/.././..////?query[test][treas][ds][]=qwert#rest");
			urls.emplace_back("http://localhost");
			urls.emplace_back("йакреведко.рф");

			for (auto &it : urls) {
				// std::cout << "Url: " << it << "\n";
				StringView r(it);
				if (search::parseUrl(r, [&] (StringView r, search::UrlToken tok) {
					/* std::cout << "\t'" << r << "' ";
					switch (tok) {
					case search::UrlParserToken::Scheme: std::cout << "Scheme"; break;
					case search::UrlParserToken::User: std::cout << "User"; break;
					case search::UrlParserToken::Password: std::cout << "Password"; break;
					case search::UrlParserToken::Host: std::cout << "Host"; break;
					case search::UrlParserToken::Port: std::cout << "Port"; break;
					case search::UrlParserToken::Path: std::cout << "Path"; break;
					case search::UrlParserToken::Query: std::cout << "Query"; break;
					case search::UrlParserToken::Fragment: std::cout << "Fragment"; break;
					case search::UrlParserToken::Blank: std::cout << "Blank"; break;
					}
					// std::cout << '\n';*/
				})) {
					if (!r.empty()) {
						return false;
					} else {
						UrlView v(it);
						auto viewString = v.get<memory::PoolInterface>();
						if (viewString != it) {
							stream << viewString << "!= " << it;
							return false;
						}
					}
				} else {
					stream << "Failed: " << it;
					return false;
				}
			}

			return true;
		});

		_desc = stream.str();

		return count == passed;
	}
} _SearchTest;

NS_SP_END
