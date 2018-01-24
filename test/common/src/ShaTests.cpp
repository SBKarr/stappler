// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "Test.h"

NS_SP_BEGIN

// test vectors from https://tools.ietf.org/html/rfc4231#section-4.1

struct ShaTest : Test {
	ShaTest() : Test("ShaTest") { }

	virtual bool run() override {
		StringStream stream;

		uint32_t failed = 0;

		stream << "\n";
		if (!test(stream, 1,
				"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",

				"4869205468657265",

				"b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7",

				"87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cde"
				"daa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854")) {
			++ failed;
		}

		if (!test(stream, 2,
				"4a656665",

				"7768617420646f2079612077616e7420666f72206e6f7468696e673f",

				"5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843",

				"164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea250554"
				"9758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737")) {
			++ failed;
		}

		if (!test(stream, 3,
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",

				"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
				"dddddddddddddddddddddddddddddddddddd",

				"773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe",

				"fa73b0089d56a284efb0f0756c890be9b1b5dbdd8ee81a3655f83e33b2279d39"
				"bf3e848279a722c806b485a47e67c807b946a337bee8942674278859e13292fb")) {
			++ failed;
		}

		if (!test(stream, 4,
				"0102030405060708090a0b0c0d0e0f10111213141516171819",

				"cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
				"cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd",

				"82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b",

				"b0ba465637458c6990e5a8c5f61d4af7e576d97ff94b872de76f8050361ee3db"
				"a91ca5c11aa25eb4d679275cc5788063a5f19741120c4f2de2adebeb10a298dd")) {
			++ failed;
		}

		if (!test(stream, 5,
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaa",

				"54657374205573696e67204c6172676572205468616e20426c6f636b2d53697a"
				"65204b6579202d2048617368204b6579204669727374",

				"60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54",

				"80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f352"
				"6b56d037e05f2598bd0fd2215d6a1e5295e64f73f63f0aec8b915a985d786598")) {
			++ failed;
		}

		if (!test(stream, 6,
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
				"aaaaaa",

				"5468697320697320612074657374207573696e672061206c6172676572207468"
				"616e20626c6f636b2d73697a65206b657920616e642061206c61726765722074"
				"68616e20626c6f636b2d73697a6520646174612e20546865206b6579206e6565"
				"647320746f20626520686173686564206265666f7265206265696e6720757365"
				"642062792074686520484d414320616c676f726974686d2e",

				"9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2",

				"e37b6a775dc87dbaa4dfa9f96e5e3ffddebd71f8867289865df5a32d20cdc944"
				"b6022cac3c4982b10d5eeb55c3e4de15134676fb6de0446065c97440fa8c6a58")) {
			++ failed;
		}

		_desc = stream.str();
		return failed == 0;
	}

	bool test(StringStream &stream, uint32_t n, const StringView &keydata, const StringView &sourcedata, const StringView &res256data, const StringView &res512data) {
		bool sha256Test = false;
		bool sha512Test = false;

		auto key = base16::decode(keydata);
		auto data = base16::decode(sourcedata);

		auto sha256 = string::Sha256::hmac(data, key);
		auto res256 = base16::decode(res256data);
		if (memcmp(sha256.data(), res256.data(), sha256.size()) == 0) {
			stream << "\tTest " << n << " (SHA-256): success\n";
			sha256Test = true;
		} else {
			stream << "\tTest " << n << " (SHA-256): failed\n";
			stream << "\t\t" << res256data << "\n\t\t" << base16::encode(sha256) << "\n";
		}

		auto sha512 = string::Sha512::hmac(data, key);
		auto res512 = base16::decode(res512data);
		if (memcmp(sha512.data(), res512.data(), sha512.size()) == 0) {
			stream << "\tTest " << n << " (SHA-512): success\n";
			sha512Test = true;
		} else {
			stream << "\tTest " << n << " (SHA-512): failed\n";
			stream << "\t\t" << res512data << "\n\t\t" << base16::encode(sha512) << "\n";
		}
		return sha256Test && sha512Test;
	}

} _ShaTest;

NS_SP_END
