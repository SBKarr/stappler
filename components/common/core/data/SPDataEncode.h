/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_DATA_SPDATAENCODE_H_
#define COMMON_DATA_SPDATAENCODE_H_

#include "SPFilesystem.h"
#include "SPDataEncodeCbor.h"
#include "SPDataEncodeJson.h"
#include "SPDataEncodeSerenity.h"

NS_SP_EXT_BEGIN(data)

struct EncodeFormat {
	static int EncodeStreamIndex;

	enum Format {
		Json				= 0b0000, // Raw JSON data, with no whitespace
		Pretty				= 0b0001, // Pretty-printed JSON data
		Cbor				= 0b0010, // CBOR data (http://cbor.io/, http://tools.ietf.org/html/rfc7049)
		DefaultFormat		= 0b0011,
		Serenity			= 0b0100,
		SerenityPretty		= 0b0101,
		PrettyTime			= 0b1001, // Pretty-printed JSON data (with time markers comment)
	};

	// We use LZ4 for compression, it's very fast to decode
	enum Compression {
		NoCompression		= 0b0000 << 4,
		LowCompression		= 0b0001 << 4,
		MediumCompression	= 0b0010 << 4,
		HighCompression		= 0b0011 << 4, // LZ4-HC

		DefaultCompress = NoCompression
	};

	enum Encryption {
		Unencrypted			= 0b0000 << 8,
		Encrypted			= 0b0001 << 8
	};

	EncodeFormat(Format fmt = DefaultFormat, Compression cmp = DefaultCompress, Encryption enc = Unencrypted, const String &key = "")
	: format(fmt), compression(cmp), encryption(enc) { }

	explicit EncodeFormat(long flag)
	: format((Format)(flag & 0x0F)), compression((Compression)(flag & 0xF0))
	, encryption((Encryption)(flag &0xF00)) { }

	EncodeFormat(const EncodeFormat & other) : format(other.format), compression(other.compression)
	, encryption(other.encryption) { }

	EncodeFormat & operator=(const EncodeFormat & other) {
		format = other.format;
		compression = other.compression;
		encryption = other.encryption;
		return *this;
	}

	bool isRaw() const {
		return compression == NoCompression && encryption == Unencrypted;
	}

	bool isTextual() const {
		return isRaw() && (format == Json || format == Pretty);
	}

	int flag() const {
		return (int)format | (int)compression | (int)encryption;
	}

	Format format;
	Compression compression;
	Encryption encryption;
};


template <typename Interface>
struct EncodeTraits {
	using InterfaceType = Interface;
	using ValueType = ValueTemplate<Interface>;
	using BytesType = typename ValueType::BytesType;
	using StringType = typename ValueType::StringType;

	static BytesType write(const ValueType &data, EncodeFormat fmt) {
		if (fmt.isRaw()) {
			switch (fmt.format) {
			case EncodeFormat::Json:
			case EncodeFormat::Pretty:
			case EncodeFormat::PrettyTime:
			{
				StringType s = json::write(data, (fmt.format == EncodeFormat::Pretty), (fmt.format == EncodeFormat::PrettyTime));
				BytesType ret; ret.reserve(s.length());
				ret.assign(s.begin(), s.end());
				return ret;
			}
				break;
			case EncodeFormat::Cbor:
			case EncodeFormat::DefaultFormat:
				return cbor::write(data);
				break;

			case EncodeFormat::Serenity:
			case EncodeFormat::SerenityPretty:
			{
				StringType s = serenity::write(data, (fmt.format == EncodeFormat::SerenityPretty));
				BytesType ret; ret.reserve(s.length());
				ret.assign(s.begin(), s.end());
				return ret;
			}
				break;
			}
		}
		return BytesType();
	}

	static bool write(std::ostream &stream, const ValueType &data, EncodeFormat fmt) {
		if (fmt.isRaw()) {
			switch (fmt.format) {
			case EncodeFormat::Json: json::write(stream, data, false); return true; break;
			case EncodeFormat::Pretty: json::write(stream, data, true); return true; break;
			case EncodeFormat::PrettyTime: json::write(stream, data, true, true); return true; break;
			case EncodeFormat::Cbor:
			case EncodeFormat::DefaultFormat:
				return cbor::write(stream, data);
				break;
			case EncodeFormat::Serenity: serenity::write(stream, data, false); return true; break;
			case EncodeFormat::SerenityPretty: serenity::write(stream, data, true); return true; break;
			}
		}
		return false;
	}

	static bool save(const ValueType &data, const StringView &file, EncodeFormat fmt) {
		const String &path = filepath::absolute(file, true);
		if (fmt.format == EncodeFormat::DefaultFormat) {
			auto ext = filepath::lastExtension(path);
			if (ext == "json") {
				fmt.format = EncodeFormat::Json;
			} else {
				fmt.format = EncodeFormat::Cbor;
			}
		}
		if (fmt.isRaw()) {
			switch (fmt.format) {
			case EncodeFormat::Json: return json::save(data, path, false); break;
			case EncodeFormat::Pretty: return json::save(data, path, true); break;
			case EncodeFormat::PrettyTime: return json::save(data, path, true, true); break;
			case EncodeFormat::Cbor:
			case EncodeFormat::DefaultFormat:
				return cbor::save(data, path);
				break;
			case EncodeFormat::Serenity: return serenity::save(data, path, false); break;
			case EncodeFormat::SerenityPretty: return serenity::save(data, path, true); break;
			}
		}
		return false;
	}
};

template <typename Interface> inline auto
write(const ValueTemplate<Interface> &data, EncodeFormat fmt = EncodeFormat()) -> typename ValueTemplate<Interface>::BytesType {
	return EncodeTraits<Interface>::write(data, fmt);
}

template <typename Interface> inline bool
write(std::ostream &stream, const ValueTemplate<Interface> &data, EncodeFormat fmt = EncodeFormat()) {
	return EncodeTraits<Interface>::write(stream, data, fmt);
}

template <typename Interface> inline bool
save(const ValueTemplate<Interface> &data, const StringView &file, EncodeFormat fmt = EncodeFormat()) {
	return EncodeTraits<Interface>::save(data, file, fmt);
}

template <typename Interface> inline auto
toString(const ValueTemplate<Interface> &data, bool pretty = false) -> typename ValueTemplate<Interface>::StringType  {
	return json::write<Interface>(data, pretty);
}
template <typename Interface> inline auto
toString(const ValueTemplate<Interface> &data, EncodeFormat::Format fmt) -> typename ValueTemplate<Interface>::StringType  {
	switch (fmt) {
	case EncodeFormat::Json:
		return json::write<Interface>(data, false);
		break;
	case EncodeFormat::Pretty:
		return json::write<Interface>(data, true);
		break;
	case EncodeFormat::PrettyTime:
		return json::write<Interface>(data, true, true);
		break;
	case EncodeFormat::Cbor:
		return base64::encode<Interface>(cbor::write(data));
		break;
	case EncodeFormat::DefaultFormat:
		return json::write<Interface>(data, false);
		break;
	case EncodeFormat::Serenity:
		return serenity::write<Interface>(data, false);
		break;
	case EncodeFormat::SerenityPretty:
		return serenity::write<Interface>(data, true);
		break;
	}
	return typename ValueTemplate<Interface>::StringType();
}

template<typename CharT, typename Traits> inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits> & stream, EncodeFormat f) {
	stream.iword( EncodeFormat::EncodeStreamIndex ) = f.flag();
	return stream;
}

template<typename CharT, typename Traits> inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits> & stream, EncodeFormat::Format f) {
	EncodeFormat fmt(f);
	stream << fmt;
	return stream;
}

template<typename CharT, typename Traits, typename Interface> inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits> & stream, const ValueTemplate<Interface> &val) {
	EncodeFormat fmt(stream.iword( EncodeFormat::EncodeStreamIndex ));
	write<Interface>(stream, val, fmt);
	return stream;
}

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATAENCODE_H_ */
