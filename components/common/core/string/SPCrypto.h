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

#ifndef COMPONENTS_COMMON_CORE_STRING_SPCRYPTO_H_
#define COMPONENTS_COMMON_CORE_STRING_SPCRYPTO_H_

#include "SPBytesView.h"
#include "SPIO.h"

struct gnutls_pubkey_st;
typedef struct gnutls_pubkey_st *gnutls_pubkey_t;

struct gnutls_privkey_st;
typedef struct gnutls_privkey_st *gnutls_privkey_t;

namespace stappler {

struct CoderSource {
	CoderSource(const uint8_t *d, size_t len) : _data(d, len) { }
	CoderSource(const char *d, size_t len) : _data((uint8_t *)d, len) { }
	CoderSource(const char *d) : _data((uint8_t *)d, strlen(d)) { }
	CoderSource(const StringView &d) : _data((uint8_t *)d.data(), d.size()) { }

	CoderSource(const typename memory::PoolInterface::BytesType &d) : _data(d.data(), d.size()) { }
	CoderSource(const typename memory::StandartInterface::BytesType &d) : _data(d.data(), d.size()) { }

	CoderSource(const typename memory::PoolInterface::StringType &d) : _data((const uint8_t *)d.data(), d.size()) { }
	CoderSource(const typename memory::StandartInterface::StringType &d) : _data((const uint8_t *)d.data(), d.size()) { }

	template <Endian Order>
	CoderSource(const BytesViewTemplate<Order> &d) : _data(d.data(), d.size()) { }

	CoderSource(const BytesReader<uint8_t> &d) : _data(d.data(), d.size()) { }
	CoderSource(const BytesReader<char> &d) : _data((const uint8_t *)d.data(), d.size()) { }

	template <size_t Size>
	CoderSource(const std::array<uint8_t, Size> &d) : _data(d.data(), Size) { }

	template <typename Container>
	CoderSource(const Container &d) : _data((const uint8_t *)d.data(), d.size()) { }

	CoderSource() { }

	BytesViewTemplate<Endian::Network> _data;
	size_t _offset = 0;

	CoderSource(const CoderSource &) = delete;
	CoderSource(CoderSource &&) = delete;

	CoderSource& operator=(const CoderSource &) = delete;
	CoderSource& operator=(CoderSource &&) = delete;

	const uint8_t *data() const { return _data.data() + _offset; }
	size_t size() const { return _data.size() - _offset; }
	bool empty() const { return _data.empty() || _offset == _data.size(); }

	BytesViewTemplate<Endian::Network> view() const { return _data; }

	uint8_t operator[] (size_t s) const { return _data[s + _offset]; }

	size_t read(uint8_t *buf, size_t nbytes) {
		const auto remains = _data.size() - _offset;
		if (nbytes > remains) {
			nbytes = remains;
		}
		memcpy(buf, _data.data() + _offset, nbytes);
		_offset += nbytes;
		return nbytes;
	}

	size_t seek(int64_t offset, io::Seek s) {
		switch (s) {
		case io::Seek::Current:
			if (offset + _offset > _data.size()) {
				_offset = _data.size();
			} else if (offset + int64_t(_offset) < 0) {
				_offset = 0;
			} else {
				_offset += offset;
			}
			break;
		case io::Seek::End:
			if (offset > 0) {
				_offset = _data.size();
			} else if (size_t(-offset) > _data.size()) {
				_offset = 0;
			} else {
				_offset = size_t(-offset);
			}
			break;
		case io::Seek::Set:
			if (offset < 0) {
				_offset = 0;
			} else if (size_t(offset) <= _data.size()) {
				_offset = size_t(offset);
			} else {
				_offset = _data.size();
			}
			break;
		}
		return _offset;
	}

	size_t tell() const {
		return _offset;
	}
};

}


namespace stappler::crypto {

class PublicKey;

enum class SignAlgorithm {
	RSA_SHA256,
	RSA_SHA512,
	ECDSA_SHA256,
	ECDSA_SHA512,
};

enum class KeyBits {
	_1024,
	_2048,
	_4096
};

using AesKey = std::array<uint8_t, 32>;

/* SHA-2 512-bit context
 * designed for chain use: Sha512().update(input).final() */
struct Sha512 {
	struct _Ctx {
		uint64_t length;
		uint64_t state[8];
		uint32_t curlen;
		uint8_t buf[128];
	};

	constexpr static uint32_t Length = 64;
	using Buf = std::array<uint8_t, Length>;

	static Buf make(const CoderSource &, const StringView &salt = StringView());
	static Buf hmac(const CoderSource &data, const CoderSource &key);

	template <typename ... Args>
	static Buf perform(Args && ... args);

	Sha512();
	Sha512 & init();

	Sha512 & update(const uint8_t *, size_t);
	Sha512 & update(const CoderSource &);

	template  <typename T, typename ... Args>
	void _update(T && t, Args && ... args);

	template  <typename T>
	void _update(T && t);

	Buf final();
	void final(uint8_t *);

	_Ctx ctx;
};

/* SHA-2 256-bit context
 * designed for chain use: Sha256().update(input).final() */
struct Sha256 {
	struct _Ctx {
		uint64_t length;
		uint32_t state[8];
		uint32_t curlen;
		uint8_t buf[64];
	};

	constexpr static uint32_t Length = 32;
	using Buf = std::array<uint8_t, Length>;

	static Buf make(const CoderSource &, const StringView &salt = StringView());
	static Buf hmac(const CoderSource &data, const CoderSource &key);

	template <typename ... Args>
	static Buf perform(Args && ... args);

	Sha256();
	Sha256 & init();

	Sha256 & update(const uint8_t *, size_t);
	Sha256 & update(const CoderSource &);

	template  <typename T, typename ... Args>
	void _update(T && t, Args && ... args);

	template  <typename T>
	void _update(T && t);

	Buf final();
	void final(uint8_t *);

	_Ctx ctx;
};

class PrivateKey {
public:
	PrivateKey();
	PrivateKey(BytesView, const CoderSource & passwd = CoderSource());
	~PrivateKey();

	bool generate(KeyBits = KeyBits::_2048);
	bool import(BytesView, const CoderSource & passwd = CoderSource());

	PublicKey exportPublic() const;

	gnutls_privkey_t getKey() const { return _key; }

	operator bool () const { return _valid; }

	template <typename Interface = memory::DefaultInterface>
	auto exportPem() -> typename Interface::BytesType;

	template <typename Interface = memory::DefaultInterface>
	auto exportDer() -> typename Interface::BytesType;

	template <typename Interface = memory::DefaultInterface>
	auto sign(BytesView, SignAlgorithm = SignAlgorithm::RSA_SHA512) -> typename Interface::BytesType;

protected:
	bool _loaded = false;
	bool _valid = false;
	gnutls_privkey_t _key;
};

class PublicKey {
public:
	PublicKey();
	PublicKey(BytesView);
	PublicKey(const PrivateKey &);
	~PublicKey();

	bool import(BytesView);
	bool import(BytesView, BytesView);

	gnutls_pubkey_t getKey() const { return _key; }

	operator bool () const { return _valid; }

	template <typename Interface = memory::DefaultInterface>
	auto exportPem() -> typename Interface::BytesType;

	template <typename Interface = memory::DefaultInterface>
	auto exportDer() -> typename Interface::BytesType;

	bool verify(BytesView data, BytesView signature, SignAlgorithm);

protected:
	bool _loaded = false;
	bool _valid = false;
	gnutls_pubkey_t _key;
};

template <typename Interface = memory::DefaultInterface>
auto encryptAes(const AesKey &, BytesView, uint32_t version) -> typename Interface::BytesType;

template <typename Interface = memory::DefaultInterface>
auto decryptAes(const AesKey &, BytesView) -> typename Interface::BytesType;

AesKey makeAesKey(BytesView pkey, BytesView hash, uint32_t version);
uint32_t getAesVersion(BytesView);

template <typename Interface = memory::DefaultInterface>
auto sign(gnutls_privkey_t, BytesView, SignAlgorithm) -> typename Interface::BytesType;

template <typename Interface = memory::DefaultInterface>
auto exportPem(gnutls_pubkey_t) -> typename Interface::BytesType;

template <typename Interface = memory::DefaultInterface>
auto exportDer(gnutls_pubkey_t) -> typename Interface::BytesType;

template <typename Interface = memory::DefaultInterface>
auto exportPem(gnutls_privkey_t) -> typename Interface::BytesType;

template <typename Interface = memory::DefaultInterface>
auto exportDer(gnutls_privkey_t) -> typename Interface::BytesType;

template <typename Interface>
auto PrivateKey::exportPem() -> typename Interface::BytesType {
	return crypto::exportPem<Interface>(_key);
}

template <typename Interface>
auto PrivateKey::exportDer() -> typename Interface::BytesType {
	return crypto::exportDer<Interface>(_key);
}

template <typename Interface>
auto PrivateKey::sign(BytesView data, SignAlgorithm algo) -> typename Interface::BytesType {
	return crypto::sign(_key, data, algo);
}

template <typename Interface>
auto PublicKey::exportPem() -> typename Interface::BytesType {
	return crypto::exportPem<Interface>(_key);
}

template <typename Interface>
auto PublicKey::exportDer() -> typename Interface::BytesType {
	return crypto::exportDer<Interface>(_key);
}

template <typename ... Args>
inline Sha512::Buf Sha512::perform(Args && ... args) {
	Sha512 ctx;
	ctx._update(std::forward<Args>(args)...);
	return ctx.final();
}

template  <typename T, typename ... Args>
inline void Sha512::_update(T && t, Args && ... args) {
	update(std::forward<T>(t));
	_update(std::forward<Args>(args)...);
}

template  <typename T>
inline void Sha512::_update(T && t) {
	update(std::forward<T>(t));
}

template <typename ... Args>
inline Sha256::Buf Sha256::perform(Args && ... args) {
	Sha256 ctx;
	ctx._update(std::forward<Args>(args)...);
	return ctx.final();
}

template  <typename T, typename ... Args>
inline void Sha256::_update(T && t, Args && ... args) {
	update(std::forward<T>(t));
	_update(std::forward<Args>(args)...);
}

template  <typename T>
inline void Sha256::_update(T && t) {
	update(std::forward<T>(t));
}


}


namespace stappler::io {

template <>
struct ProducerTraits<CoderSource> {
	using type = CoderSource;
	static size_t ReadFn(void *ptr, uint8_t *buf, size_t nbytes) {
		return ((type *)ptr)->read(buf, nbytes);
	}

	static size_t SeekFn(void *ptr, int64_t offset, Seek s) {
		return ((type *)ptr)->seek(offset, s);
	}
	static size_t TellFn(void *ptr) {
		return ((type *)ptr)->tell();
	}
};

}

#endif /* COMPONENTS_COMMON_CORE_STRING_SPCRYPTO_H_ */
