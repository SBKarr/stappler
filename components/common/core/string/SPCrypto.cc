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

#include "SPCrypto.h"

#include <gnutls/abstract.h>
#include <gnutls/crypto.h>

namespace stappler::crypto {

static bool isPemKey(BytesView data) {
	StringView str((const char *)data.data(), data.size());
	str.readChars<StringView::WhiteSpace>();
	if (str.is("-----")) {
		return true;
	}
	return false;
}

static bool exportPubKeyDer(gnutls_pubkey_t key, const Callback<void(gnutls_datum_t *)> &cb) {
	gnutls_datum_t out;
	if (gnutls_pubkey_export2(key, GNUTLS_X509_FMT_DER, &out) == GNUTLS_E_SUCCESS) {
		cb(&out);
		gnutls_free(out.data);
		return true;
	}
	return false;
}

static bool exportPubKeyPem(gnutls_pubkey_t key, const Callback<void(gnutls_datum_t *)> &cb) {
	gnutls_datum_t out;
	if (gnutls_pubkey_export2(key, GNUTLS_X509_FMT_PEM, &out) == GNUTLS_E_SUCCESS) {
		cb(&out);
		gnutls_free(out.data);
		return true;
	}
	return false;
}

static bool exportPrivKeyDer(gnutls_privkey_t key, const Callback<void(gnutls_datum_t *)> &cb) {
	bool success = false;
	gnutls_datum_t out;
	gnutls_x509_privkey_t pk;
	if (gnutls_privkey_export_x509(key, &pk) == GNUTLS_E_SUCCESS) {
		if (gnutls_x509_privkey_export2_pkcs8(pk, GNUTLS_X509_FMT_DER, nullptr, 0, &out) == GNUTLS_E_SUCCESS) {
			cb(&out);
			gnutls_free(out.data);
			success = true;
		}
		gnutls_x509_privkey_deinit(pk);
	}
	return success;
}

static bool exportPrivKeyPem(gnutls_privkey_t key, const Callback<void(gnutls_datum_t *)> &cb) {
	bool success = false;
	gnutls_datum_t out;
	gnutls_x509_privkey_t pk;
	if (gnutls_privkey_export_x509(key, &pk) == GNUTLS_E_SUCCESS) {
		if (gnutls_x509_privkey_export2_pkcs8(pk, GNUTLS_X509_FMT_PEM, nullptr, 0, &out) == GNUTLS_E_SUCCESS) {
			cb(&out);
			gnutls_free(out.data);
			success = true;
		}
		gnutls_x509_privkey_deinit(pk);
	}
	return success;
}

static gnutls_sign_algorithm_t getAlgo(SignAlgorithm a) {
	switch (a) {
	case SignAlgorithm::RSA_SHA256: return GNUTLS_SIGN_RSA_SHA256; break;
	case SignAlgorithm::RSA_SHA512: return GNUTLS_SIGN_RSA_SHA512; break;
	case SignAlgorithm::ECDSA_SHA256: return GNUTLS_SIGN_ECDSA_SHA256; break;
	case SignAlgorithm::ECDSA_SHA512: return GNUTLS_SIGN_ECDSA_SHA512; break;
	}
	return GNUTLS_SIGN_UNKNOWN;
}

static constexpr size_t DATA_ALIGN_BOUNDARY ( 16 );

static bool doEncryptAes(const AesKey &key, BytesView d, size_t dataSize, BytesView output, uint32_t version) {
	memcpy((uint8_t *)output.data(), &dataSize, sizeof(dataSize));
	memcpy((uint8_t *)output.data() + 8, &version, sizeof(version));
	memcpy((uint8_t *)output.data() + 16, d.data(), d.size());

	uint8_t iv[16] = { 0 };
	gnutls_datum_t ivData;
	ivData.data = (unsigned char *)iv;
	ivData.size = (unsigned int)16;

	gnutls_datum_t keyData;
	keyData.data = (unsigned char *)key.data();
	keyData.size = (unsigned int)key.size();

	auto algo = GNUTLS_CIPHER_AES_256_CBC;

	gnutls_cipher_hd_t aes;
	auto err = gnutls_cipher_init(&aes, algo, &keyData, &ivData);
	if (err != 0) {
		std::cout << "Crypto: gnutls_cipher_init() = [" << err << "] " << gnutls_strerror(err) << "\n";
		return false;
	}

	size_t outSize = output.size() - 16;
	err = gnutls_cipher_encrypt(aes, (uint8_t *)output.data() + 16, outSize);
	if (err != 0) {
		gnutls_cipher_deinit(aes);
		std::cout << "Crypto: gnutls_cipher_encrypt() = [" << err << "] " << gnutls_strerror(err) << "\n";
		return false;
	}

	gnutls_cipher_deinit(aes);
	return true;
}

static bool doDecryptAes(const AesKey &key, BytesView val, BytesView output) {
	uint8_t iv[16] = { 0 };
	gnutls_datum_t ivData;
	ivData.data = (unsigned char *)iv;
	ivData.size = (unsigned int)16;

	gnutls_datum_t keyData;
	keyData.data = (unsigned char *)key.data();
	keyData.size = (unsigned int)key.size();

	auto algo = GNUTLS_CIPHER_AES_256_CBC;

	gnutls_cipher_hd_t aes;
	auto err = gnutls_cipher_init(&aes, algo, &keyData, &ivData);
	if (err != 0) {
		std::cout << "Crypto: gnutls_cipher_init() = [" << err << "] " << gnutls_strerror(err) << "\n";
		return false;
	}

	err = gnutls_cipher_decrypt2(aes, val.data(), val.size(), (uint8_t *)output.data(), output.size());
	if (err != 0) {
		gnutls_cipher_deinit(aes);
		std::cout << "Crypto: gnutls_pubkey_init() = [" << err << "] " << gnutls_strerror(err) << "\n";
		return false;
	}

	gnutls_cipher_deinit(aes);
	return true;
}

template <>
auto encryptAes<memory::PoolInterface>(const AesKey &key, BytesView d, uint32_t version) -> memory::PoolInterface::BytesType {
	auto dataSize = d.size();
	auto blockSize = math::align<size_t>(dataSize, DATA_ALIGN_BOUNDARY);
	memory::PoolInterface::BytesType output; output.resize(blockSize + 16);

	if (!doEncryptAes(key, d, dataSize, output, version)) {
		return memory::PoolInterface::BytesType();
	}

	return output;
}

template <>
auto encryptAes<memory::StandartInterface>(const AesKey &key, BytesView d, uint32_t version) -> memory::StandartInterface::BytesType {
	auto dataSize = d.size();
	auto blockSize = math::align<size_t>(dataSize, DATA_ALIGN_BOUNDARY);
	memory::StandartInterface::BytesType output; output.resize(blockSize + 16);

	if (!doEncryptAes(key, d, dataSize, output, version)) {
		return memory::StandartInterface::BytesType();
	}

	return output;
}

template <>
auto decryptAes<memory::PoolInterface>(const AesKey &key, BytesView val) -> memory::PoolInterface::BytesType {
	auto dataSize = val.readUnsigned64();
	auto blockSize = math::align<size_t>(dataSize, DATA_ALIGN_BOUNDARY);

	val.offset(8);

	memory::PoolInterface::BytesType output; output.resize(blockSize);

	if (!doDecryptAes(key, val, output)) {
		return memory::PoolInterface::BytesType();
	}

	output.resize(dataSize);
	return output;
}

template <>
auto decryptAes<memory::StandartInterface>(const AesKey &key, BytesView val) -> memory::StandartInterface::BytesType {
	auto dataSize = val.readUnsigned64();
	auto blockSize = math::align<size_t>(dataSize, DATA_ALIGN_BOUNDARY);

	val.offset(8);

	memory::StandartInterface::BytesType output; output.resize(blockSize);

	if (!doDecryptAes(key, val, output)) {
		return memory::StandartInterface::BytesType();
	}

	output.resize(dataSize);
	return output;
}

AesKey makeAesKey(BytesView pkey, BytesView hash, uint32_t version) {
	if (version == 1) {
		string::Sha256::Buf ret;
		crypto::PrivateKey pk(pkey);

		auto d = pk.sign(hash, crypto::SignAlgorithm::RSA_SHA512);
		if (!d.empty()) {
			d.resize(256);
			ret = string::Sha256().update(d).final();
		} else {
			ret = string::Sha256().update(hash).update(pkey).final();
		}
		return ret;
	} else {
		return string::Sha256().update(hash).update(pkey).final();
	}
}

uint32_t getAesVersion(BytesView val) {
	val.offset(8);
	return val.readUnsigned32();
}

PrivateKey::PrivateKey() {
	if (gnutls_privkey_init( &_key ) == GNUTLS_E_SUCCESS) {
		_valid = true;
	}
}

PrivateKey::PrivateKey(BytesView data, const CoderSource &str) {
	if (gnutls_privkey_init( &_key ) == GNUTLS_E_SUCCESS) {
		_valid = true;

		import(data, str);
	}
}

PrivateKey::~PrivateKey() {
	if (_valid) {
		gnutls_privkey_deinit( _key );
		_valid = false;
	}
}

bool PrivateKey::generate(KeyBits bits) {
	if (_valid && !_loaded) {
		int err = 0;
		switch (bits) {
		case KeyBits::_1024: err = gnutls_privkey_generate(_key, GNUTLS_PK_RSA, 1024, 0); break;
		case KeyBits::_2048: err = gnutls_privkey_generate(_key, GNUTLS_PK_RSA, 2048, 0); break;
		case KeyBits::_4096: err = gnutls_privkey_generate(_key, GNUTLS_PK_RSA, 4096, 0); break;
		}

		if (err == GNUTLS_E_SUCCESS) {
			_loaded = true;
			return true;
		} else {
			gnutls_privkey_deinit( _key );
			_valid = false;
		}
	}

	return false;
}

bool PrivateKey::import(BytesView data, const CoderSource &passwd) {
	if (_valid && !_loaded) {
		gnutls_datum_t keyData;
		keyData.data = (unsigned char *)data.data();
		keyData.size = (unsigned int)data.size();

		if (isPemKey(data)) {
			if (gnutls_privkey_import_x509_raw(_key, &keyData, GNUTLS_X509_FMT_PEM,
					(const char *)(passwd._data.empty() ? nullptr : passwd._data.data()), 0) != GNUTLS_E_SUCCESS) {
				gnutls_privkey_deinit( _key );
				_valid = false;
			} else {
				_loaded = true;
				return true;
			}
		} else {
			if (gnutls_privkey_import_x509_raw(_key, &keyData, GNUTLS_X509_FMT_DER,
					(const char *)(passwd._data.empty() ? nullptr : passwd._data.data()), 0) != GNUTLS_E_SUCCESS) {
				gnutls_privkey_deinit( _key );
				_valid = false;
			} else {
				_loaded = true;
				return true;
			}
		}
	}
	return false;
}


PublicKey PrivateKey::exportPublic() const {
	return PublicKey(*this);
}

template <>
auto sign<memory::PoolInterface>(gnutls_privkey_t _key, BytesView data, SignAlgorithm algo) -> memory::PoolInterface::BytesType {
	memory::PoolInterface::BytesType ret;

	gnutls_datum_t dataToSign;
	dataToSign.data = (unsigned char *)data.data();
	dataToSign.size = (unsigned int)data.size();

	gnutls_datum_t signature;

	if (gnutls_privkey_sign_data2(_key, getAlgo(algo), 0, &dataToSign, &signature) == GNUTLS_E_SUCCESS) {
		ret.resize(signature.size);
		memcpy(ret.data(), signature.data, signature.size);
		gnutls_free(signature.data);
	}

	return ret;
}

template <>
auto sign<memory::StandartInterface>(gnutls_privkey_t _key, BytesView data, SignAlgorithm algo) -> memory::StandartInterface::BytesType {
	memory::StandartInterface::BytesType ret;

	gnutls_datum_t dataToSign;
	dataToSign.data = (unsigned char *)data.data();
	dataToSign.size = (unsigned int)data.size();

	gnutls_datum_t signature;

	if (gnutls_privkey_sign_data2(_key, getAlgo(algo), 0, &dataToSign, &signature) == GNUTLS_E_SUCCESS) {
		ret.resize(signature.size);
		memcpy(ret.data(), signature.data, signature.size);
		gnutls_free(signature.data);
	}

	return ret;
}

PublicKey::PublicKey() {
	auto err = gnutls_pubkey_init( &_key );
	if (err == GNUTLS_E_SUCCESS) {
		_valid = true;
	} else {
		std::cout << "Crypto: gnutls_pubkey_init() = [" << err << "] " << gnutls_strerror(err) << "\n";
	}
}
PublicKey::PublicKey(BytesView data) {
	auto err = gnutls_pubkey_init( &_key );
	if (err == GNUTLS_E_SUCCESS) {
		_valid = true;

		import(data);
	} else {
		std::cout << "Crypto: gnutls_pubkey_init() = [" << err << "] " << gnutls_strerror(err) << "\n";
	}
}
PublicKey::PublicKey(const PrivateKey &priv) {
	auto err = gnutls_pubkey_init( &_key );
	if (err == GNUTLS_E_SUCCESS) {
		_valid = true;

		if (gnutls_pubkey_import_privkey(_key, priv.getKey(), 0, 0) != GNUTLS_E_SUCCESS) {
			gnutls_pubkey_deinit( _key );
			_valid = false;
		} else {
			_loaded = true;
		}
	} else {
		std::cout << "Crypto: gnutls_pubkey_init() = [" << err << "] " << gnutls_strerror(err) << "\n";
	}
}

PublicKey::~PublicKey() {
	if (_valid) {
		gnutls_pubkey_deinit( _key );
		_valid = false;
	}
}

bool PublicKey::import(BytesView data) {
	if (_valid && !_loaded) {
		gnutls_datum_t keyData;
		keyData.data = (unsigned char *)data.data();
		keyData.size = (unsigned int)data.size();

		int err = 0;
		if (isPemKey(data)) {
			err = gnutls_pubkey_import(_key, &keyData, GNUTLS_X509_FMT_PEM);
			if (err != GNUTLS_E_SUCCESS) {
				gnutls_pubkey_deinit( _key );
				_valid = false;
			} else {
				_loaded = true;
				return true;
			}
		} else {
			err = gnutls_pubkey_import(_key, &keyData, GNUTLS_X509_FMT_DER);
			if (err != GNUTLS_E_SUCCESS) {
				gnutls_pubkey_deinit( _key );
				_valid = false;
			} else {
				_loaded = true;
				return true;
			}
		}
	}
	return false;
}

bool PublicKey::import(BytesView m, BytesView e) {
	if (_valid && !_loaded) {

		gnutls_datum_t mData;
		mData.data = (unsigned char *)m.data();
		mData.size = (unsigned int)m.size();

		gnutls_datum_t eData;
		eData.data = (unsigned char *)e.data();
		eData.size = (unsigned int)e.size();

		auto err = gnutls_pubkey_import_rsa_raw(_key, &mData, &eData);
		if (err != GNUTLS_E_SUCCESS) {
			gnutls_pubkey_deinit( _key );
			_valid = false;
		} else {
			_loaded = true;
			return true;
		}
	}

	return false;
}

bool PublicKey::verify(BytesView data, BytesView signature, SignAlgorithm algo) {
	gnutls_datum_t inputData;
	inputData.data = (unsigned char *)data.data();
	inputData.size = (unsigned int)data.size();

	gnutls_datum_t signatureData;
	signatureData.data = (unsigned char *)signature.data();
	signatureData.size = (unsigned int)signature.size();

	return gnutls_pubkey_verify_data2(_key, getAlgo(algo), 0, &inputData, &signatureData) >= 0;
}

template <>
auto exportPem<memory::PoolInterface>(gnutls_pubkey_t _key) -> memory::PoolInterface::BytesType {
	memory::PoolInterface::BytesType ret;
	exportPubKeyPem(_key, [&] (gnutls_datum_t *data) {
		ret.resize(data->size);
		memcpy(ret.data(), data->data, data->size);
	});
	return ret;
}

template <>
auto exportPem<memory::StandartInterface>(gnutls_pubkey_t _key) -> memory::StandartInterface::BytesType {
	memory::StandartInterface::BytesType ret;
	exportPubKeyPem(_key, [&] (gnutls_datum_t *data) {
		ret.resize(data->size);
		memcpy(ret.data(), data->data, data->size);
	});
	return ret;
}

template <>
auto exportDer<memory::PoolInterface>(gnutls_pubkey_t _key) -> memory::PoolInterface::BytesType {
	memory::PoolInterface::BytesType ret;
	exportPubKeyDer(_key, [&] (gnutls_datum_t *data) {
		ret.resize(data->size);
		memcpy(ret.data(), data->data, data->size);
	});
	return ret;
}

template <>
auto exportDer<memory::StandartInterface>(gnutls_pubkey_t _key) -> memory::StandartInterface::BytesType {
	memory::StandartInterface::BytesType ret;
	exportPubKeyDer(_key, [&] (gnutls_datum_t *data) {
		ret.resize(data->size);
		memcpy(ret.data(), data->data, data->size);
	});
	return ret;
}

template <>
auto exportPem<memory::PoolInterface>(gnutls_privkey_t _key) -> memory::PoolInterface::BytesType {
	memory::PoolInterface::BytesType ret;
	exportPrivKeyPem(_key, [&] (gnutls_datum_t *data) {
		ret.resize(data->size);
		memcpy(ret.data(), data->data, data->size);
	});
	return ret;
}

template <>
auto exportPem<memory::StandartInterface>(gnutls_privkey_t _key) -> memory::StandartInterface::BytesType {
	memory::StandartInterface::BytesType ret;
	exportPrivKeyPem(_key, [&] (gnutls_datum_t *data) {
		ret.resize(data->size);
		memcpy(ret.data(), data->data, data->size);
	});
	return ret;
}

template <>
auto exportDer<memory::PoolInterface>(gnutls_privkey_t _key) -> memory::PoolInterface::BytesType {
	memory::PoolInterface::BytesType ret;
	exportPrivKeyDer(_key, [&] (gnutls_datum_t *data) {
		ret.resize(data->size);
		memcpy(ret.data(), data->data, data->size);
	});
	return ret;
}

template <>
auto exportDer<memory::StandartInterface>(gnutls_privkey_t _key) -> memory::StandartInterface::BytesType {
	memory::StandartInterface::BytesType ret;
	exportPrivKeyDer(_key, [&] (gnutls_datum_t *data) {
		ret.resize(data->size);
		memcpy(ret.data(), data->data, data->size);
	});
	return ret;
}

}
