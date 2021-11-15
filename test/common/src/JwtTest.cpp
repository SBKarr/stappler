/**
 Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPJsonWebToken.h"
#include "SPCrypto.h"
#include "SPValid.h"
#include "Test.h"

#include <gnutls/gnutls.h>

namespace stappler {

static constexpr auto PRIVATE_KEY =
R"PubKey(-----BEGIN RSA PRIVATE KEY-----
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
-----END RSA PRIVATE KEY-----
)PubKey";

// Открытый ключ ключ, использовается для защиты авторизации
static constexpr auto PUBLIC_KEY =
R"PubKey(-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAvcL8IPbL16loGZNKBlXG
fJc8ffZjqlYjS3fwar/GSQbl8phRQXtMeqqJzn32IEjGYQOmxRaiP+eRVFiPES6b
doP01ajv2CUPY4R/56IBm0ID+Ui774Kn0ZB3XW9UWfZUfqH172JUlOTtZ396rYEO
VcoOrnL+9kyqfdGvgbaxEhx9RDDxd4xGHeRKQxbB48j992qHgeL2XPOyGlMuHLx8
hbWmSsVL4KT24Vo+0pqvkNQh04Lz154/4EDcuAMCPrwdsclQrN+mG9PcgySBpSKR
bB4Tpgb5QkufVOloFXFEfP49I8+8vF/D6nrfvRMEMr5jJxhoFyXjCdSwP+vHoNYe
0in5/TDNij2TptcIE0dKlxlr+Q2w/ywhZ711YKOURYEM6y/kBRxHoGBB6CmZLzRb
xAoehl1WqAb4GMUmU6HKLnopJEwbQA0jFg7wOQ81Pl5XyzCWsu0miiuDU169CnW9
rnuUieft8acxcmT1XjFZQlBpj12vYxjrzOFQPwS4Nr5iYFGKaCj9AqPfoX7+5e8A
7/wW5CRWXE/NPyIRS1yPKiUTihoc0WOcUgB1Puc/S4LLl9wDY5cEpaOAU+K563rx
h+re3Rq4cSH7RMj0sePdc202ZpzVptcdiYJEjgkutNCyFAS3ghHPthjA8EKlBE9p
zziPCKD41RKzWE1H+jDXx2MCAwEAAQ==
-----END PUBLIC KEY-----
)PubKey";

static constexpr auto OPENSSH_KEY =
"ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDcgSBj1YSdYIpjQ087Gr7e5z6Y7XmY6WbjTuIezvE8MbdGIk3+0ItUATaAdIXPXHX/+7kLULeOXyZxw/VaUGu1c3TcAa9romGK1ghiFSH3f0HuYZL2dwqrPhn9ZYT/3TgQVlTMKStEBJ4qpAWtHmNqnyCPDptsjkHgQP8UYDrcvbGR6mqWKEaKqVgC551/TPiRdtRG47zFEXJkvH7r4Qgj318b1qOP2wyZ+9AlCjyZABOvCPbapSg5OlppUh2rkhF6fQVLMJEYLIwbXa8g6Fu5wMiRqS1209nkpKmYXxVeIkZf1/I7CrppeXsnABIfoWfx6Hk34Dp9JV/p6Le9KPJJ sbkarr@sbkarr-virtual-machine";

static void stappler_gnutls_log_func(int l, const char *text) {
	//std::cout << "GnuTLS[" << l << "] " << text;
}

struct JwtTest : Test {
	JwtTest() : Test("JwtTest") {
		gnutls_global_set_log_level(6);
		gnutls_global_set_log_function(&stappler_gnutls_log_func);
	}

	virtual bool run() override {
		auto bytes = valid::convertOpenSSHKey(OPENSSH_KEY);

		do {
			crypto::PublicKey pk(bytes);

			auto pemKey = pk.exportPem();
			if (pemKey.empty()) {
				return false;
			}
		} while (0);

		do {
			crypto::PublicKey pk(BytesView((const uint8_t *)PUBLIC_KEY, strlen(PUBLIC_KEY)));
			auto pemKey = pk.exportPem();
			if (pemKey.empty()) {
				return false;
			}
		} while (0);

		do {
			crypto::PrivateKey pk(BytesView((const uint8_t *)PRIVATE_KEY, strlen(PRIVATE_KEY)));
			auto pubKey = pk.exportPublic();
			auto pemKey = pubKey.exportPem();
			if (pemKey.empty()) {
				return false;
			} else {
				// std::cout << StringView((const char *)pemKey.data(), pemKey.size()) << "\n";

				auto privKey = pk.exportPem();
				// std::cout << StringView((const char *)privKey.data(), privKey.size()) << "\n";

				crypto::PrivateKey tmpPk(privKey);
				auto pubKey = tmpPk.exportPublic();
				auto pemKey = pubKey.exportPem();
				if (pemKey.empty()) {
					return false;
				} else {
					// std::cout << StringView((const char *)pemKey.data(), pemKey.size()) << "\n";
				}
			}
		} while (0);

		do {
			JsonWebToken token({
				pair("data", data::Value("data")),
				pair("int", data::Value(42)),
			});

			auto d = token.exportSigned(JsonWebToken::SigAlg::RS256, PRIVATE_KEY);
			//std::cout << d << "\n";

			JsonWebToken tmpToken(d);

			if (!tmpToken.validate(JsonWebToken::SigAlg::RS256, PUBLIC_KEY)) {
				return false;
			}

		} while (0);

		do {
			auto secret = string::Sha512::make(PUBLIC_KEY);

			AesToken::Keys keys({
				PUBLIC_KEY,
				PRIVATE_KEY,
				BytesView(secret)
			});

			AesToken tok = AesToken::create(keys);

			tok.setString(StringView("String"), "string");
			tok.setInteger(42, "integer");

			auto fp = AesToken::Fingerprint(secret);
			auto d = tok.exportData(fp);
			auto next = AesToken::parse(d, fp, keys);

			if (next.getData() != tok.getData()) {
				return false;
			}

		} while (0);

		return true;
	}
} _JwtTest;

}
