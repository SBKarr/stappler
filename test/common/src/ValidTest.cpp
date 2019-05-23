/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPTime.h"
#include "SPString.h"
#include "SPData.h"
#include "SPValid.h"
#include "Test.h"

NS_SP_BEGIN

// test vectors from https://tools.ietf.org/html/rfc4231#section-4.1

struct ValidTest : MemPoolTest {
	ValidTest() : MemPoolTest("ValidTest") { }

	virtual bool run(pool_t *) override {
		StringStream stream;

		uint32_t failed = 0;

		Value urls;
		urls.addString("localhost");
		urls.addString("localhost:8080");
		urls.addString("localhost/test1/test2");
		urls.addString("/usr/local/bin/bljad");
		urls.addString("https://localhost/test/.././..////?query[test][treas][ds][]=qwert#rest");
		urls.addString("http://localhost");
		urls.addString("йакреведко.рф");
		urls.addString("https://йакреведко.рф/test/.././..////?query[креведко][treas][ds][]=qwert#аяклешня");
		urls.addString("ssh://git@atom0.stappler.org:21002/StapplerApi.git?qwr#test");

		for (auto &it : urls.asArray()) {
			memory::string str(it.getString());
			if (!valid::validateUrl(str)) {
				stream << "Url: [invalid] " << str << "\n";
				++ failed;
			} else {
				stream << "Url: [valid] " << it.getString() << " >> " << str << "\n";
			}
		}

		Value emails;
		emails.addString("prettyandsimple@example.com");
		emails.addString("йакреведко@упячка.рф");
		emails.addString("very.common@example.com");
		emails.addString("disposable.style.email.with+symbol@example.com");
		emails.addString("other.email-with-dash@example.com");
		emails.addString("\"much.more unusual\"@example.com");
		emails.addString("\"very.unusual.@.unusual.com\"@example.com");
		emails.addString("\"very.(),:;<>[]\\\".VERY.\\\"very@\\\\ \\\"very\\\".unusual\"@strange.example.com");
		emails.addString("admin@mailserver1");
		emails.addString("#!$%&'*+-/=?^_`{}|~@example.org");
		emails.addString("\"()<>[]:,;@\\\"!#$%&'*+-/=?^_`{}| ~.a\"@example.org");
		emails.addString("\" \"@example.org");
		emails.addString("example@localhost");
		emails.addString("example@s.solutions");
		emails.addString("user@com");
		emails.addString("user@localserver");
		emails.addString("user@[IPv6:2001:db8::1]");
		emails.addString(" (test comment) user(test comment)@(test comment)localserver(test comment) ");
		emails.addString("аяклешня@йакреведко.рф");

		for (auto &it : emails.asArray()) {
			memory::string str(it.getString());
			if (!valid::validateEmail(str)) {
				stream << "Email: [invalid] " << str << "\n";
				++ failed;
			} else {
				stream << "Email: [valid] " << it.getString() << " >> " << str << "\n";
			}
		}

		emails.clear();

		// should fail
		emails.addString("john.doe@failed..com");
		emails.addString("Abc.failed.com");
		emails.addString("A@b@c@failed.com");
		emails.addString("a\"b(c)d,e:f;g<h>i[j\\k]l@failed.com"); // (none of the special characters in this local part are allowed outside quotation marks)
		emails.addString("just\"not\"right@failed.com"); // (quoted strings must be dot separated or the only element making up the local-part)
		emails.addString("this is\"not\allowed@failed.com"); // (spaces, quotes, and backslashes may only exist when within quoted strings and preceded by a backslash)
		emails.addString("this\\ still\\\"not\\allowed@failed.com"); // (even if escaped (preceded by a backslash), spaces, quotes, and backslashes must still be contained by quotes)
		emails.addString("john..doe@failed.com"); // (double dot before @)

		for (auto &it : emails.asArray()) {
			memory::string str(it.getString());
			if (!valid::validateEmail(str)) {
				stream << "Email (should fail): [invalid] " << str << "\n";
			} else {
				stream << "Email (should fail): [valid] " << it.getString() << " >> " << str << "\n";
				++ failed;
			}
		}

		_desc = stream.str();
		return failed == 0;
	}

} _ValidTest;

NS_SP_END
