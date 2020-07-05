// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPBytesView.h"
#include "SPDefine.h"
#include "SPFilesystem.h"
#include "SPData.h"
#include "SPDataStream.h"
#include "SPString.h"

#if (__APPLE__)

#include <chrono>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>

// http://stackoverflow.com/questions/28690054/how-do-i-parse-and-convert-datetime-to-the-rfc-3339-with-%D1%81
template <class Int>
Int days_from_civil(Int y, unsigned m, unsigned d) noexcept {
    static_assert(std::numeric_limits<unsigned>::digits >= 18,
             "This algorithm has not been ported to a 16 bit unsigned integer");
    static_assert(std::numeric_limits<Int>::digits >= 20,
             "This algorithm has not been ported to a 16 bit signed integer");
    y -= m <= 2;
    const Int era = (y >= 0 ? y : y-399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);      // [0, 399]
    const unsigned doy = (153*(m + (m > 2 ? -3 : 9)) + 2)/5 + d-1;  // [0, 365]
    const unsigned doe = yoe * 365 + yoe/4 - yoe/100 + doy;         // [0, 146096]
    return era * 146097 + static_cast<Int>(doe) - 719468;
}

using days = std::chrono::duration <int, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

namespace std {

namespace chrono {

template<class charT, class traits> std::basic_istream<charT,traits> &
operator >>(std::basic_istream<charT,traits>& is, system_clock::time_point& item) {
    typename std::basic_istream<charT,traits>::sentry ok(is);
    if (ok) {
        std::ios_base::iostate err = std::ios_base::goodbit;
		const std::time_get<charT>& tg = std::use_facet<std::time_get<charT> >(is.getloc());
		std::tm t = {};
		const charT pattern[] = "%Y-%m-%dT%H:%M:%S";
		tg.get(is, 0, is, err, &t, begin(pattern), end(pattern)-1);
		if (err == std::ios_base::goodbit) {
			charT sign = {};
			is.get(sign);
			err = is.rdstate();
			if (err == std::ios_base::goodbit) {
				if (sign == charT('+') || sign == charT('-')) {
					std::tm t2 = {};
					const charT pattern2[] = "%H:%M";
					tg.get(is, 0, is, err, &t2, begin(pattern2), end(pattern2)-1);
					if (!(err & std::ios_base::failbit)) {
						auto offset = (sign == charT('+') ? 1 : -1) * (hours{t2.tm_hour} + minutes{t2.tm_min});
						item = system_clock::time_point{
								days{days_from_civil(t.tm_year+1900, t.tm_mon+1, t.tm_mday)} +
								hours{t.tm_hour} + minutes{t.tm_min} + seconds{t.tm_sec} -
							offset};
					} else {
						err |= ios_base::failbit;
					}
				} else if (sign == charT('Z')) {
					item = system_clock::time_point{
						days{days_from_civil(t.tm_year+1900, t.tm_mon+1, t.tm_mday)} +
						hours{t.tm_hour} + minutes{t.tm_min} + seconds{t.tm_sec}};
				} else {
					err |= ios_base::failbit;
				}
			} else {
				err |= ios_base::failbit;
			}
		} else {
			err |= ios_base::failbit;
		}

        is.setstate(err);
    }
    return is;
}

}  // namespace chrono
}  // namespace std

NS_SP_EXT_BEGIN(platform)

template <typename T>
struct Asn1DecoderTraits {
	using success = char;
	using failure = long;

	InvokerCallTest_MakeCallTest(onBeginSet, success, failure);
	InvokerCallTest_MakeCallTest(onEndSet, success, failure);
	InvokerCallTest_MakeCallTest(onBeginSequence, success, failure);
	InvokerCallTest_MakeCallTest(onEndSequence, success, failure);
	InvokerCallTest_MakeCallTest(onOid, success, failure);
	InvokerCallTest_MakeCallTest(onNull, success, failure);
	InvokerCallTest_MakeCallTest(onInteger, success, failure);
	InvokerCallTest_MakeCallTest(onBoolean, success, failure);
	InvokerCallTest_MakeCallTest(onBytes, success, failure);
	InvokerCallTest_MakeCallTest(onString, success, failure);
	InvokerCallTest_MakeCallTest(onCustom, success, failure);
};

template <typename ReaderType, typename Traits = Asn1DecoderTraits<ReaderType>>
struct Asn1Decoder {
	enum Type : uint8_t {
		Boolean = 0x01,
		Integer = 0x02,
		BitString = 0x03,
		OctetString = 0x04,
		Null = 0x05,
		Oid = 0x06,
		Utf8String = 0x0C,
		Sequence = 0x10,
		Set = 0x11,
		PrintableString = 0x13,
		T61String = 0x14,
		AsciiString = 0x16,
		UtcTime = 0x17,
		Time = 0x18,
		UniversalString = 0x1C,
		BmpString = 0x1E,
		HighForm = 0x1F,
		Primitive = 0x00,

		ConstructedBit = 0x20,
		ContextSpecificBit = 0x80,
	};

	size_t decodeSize(DataReader<ByteOrder::Network> &r) {
		size_t size = 0;
		auto sizeByte = r.readUnsigned();
		if (sizeByte & 0x80) {
			auto count = sizeByte & (~0x80);
			switch (count) {
			case 1: size = r.readUnsigned(); break;
			case 2: size = r.readUnsigned16(); break;
			case 3: size = r.readUnsigned24(); break;
			case 4: size = r.readUnsigned32(); break;
			case 8: size = r.readUnsigned64(); break;
			default: return 0; break;
			}
		} else {
			size = sizeByte;
		}
		return size;
	}

	bool decodeValue(ReaderType &reader, DataReader<ByteOrder::Network> &r) {
		auto b = r.readUnsigned();
		Type type = Type(b & 0x1F);
		switch (type) {
		case Primitive:
			if (b & (ContextSpecificBit | ConstructedBit)) {
				return decodeAny(reader, r);
			} else {
				return decodeUnknown(reader, r, b);
			}
			break;
		case Boolean: return decodeBoolean(reader, r); break;
		case Integer: return decodeInteger(reader, r); break;
		case Oid: return decodeOid(reader, r); break;
		case Sequence: return decodeSequence(reader, r); break;
		case Set: return decodeSet(reader, r); break;
		case OctetString: return decodeOctetString(reader, r); break;
		case Null:
			r += decodeSize(r);
			if constexpr (Traits::onNull) {
				reader.onNull(*this);
			}
			return true;
			break;
		case Utf8String:
		case UniversalString:
		case AsciiString:
		case PrintableString:
		case T61String:
		case BmpString:
		case UtcTime:
		case Time:
			return decodeString(reader, r, type);
			break;
		case HighForm:
			return false;
			break;
		case BitString:
			return decodeBitString(reader, r, b & ConstructedBit);
			break;
		default:
			return decodeUnknown(reader, r, b); break;
		}
		return false;
	}

	bool decodeSequence(ReaderType &reader, DataReader<ByteOrder::Network> &r) {
		auto size = decodeSize(r);
		if (size == 0) {
			return false;
		}

		DataReader<ByteOrder::Network> nextR(r.data(), size);
		r += size;


		if constexpr (Traits::onBeginSequence) { reader.onBeginSequence(*this); }
		while(!nextR.empty()) {
			if (!decodeValue(reader, nextR)) {
				if constexpr (Traits::onEndSequence) { reader.onEndSequence(*this); }
				return false;
			}
		}
		if constexpr (Traits::onEndSequence) { reader.onEndSequence(*this); }

		return true;
	}

	bool decodeSet(ReaderType &reader, DataReader<ByteOrder::Network> &r) {
		auto size = decodeSize(r);
		if (size == 0) {
			return false;
		}

		DataReader<ByteOrder::Network> nextR(r.data(), size);
		r += size;

		if constexpr (Traits::onBeginSet) { reader.onBeginSet(*this); }
		while(!nextR.empty()) {
			if (!decodeValue(reader, nextR)) {
				if constexpr (Traits::onEndSet) { reader.onEndSet(*this); }
				return false;
			}
		}
		if constexpr (Traits::onEndSet) { reader.onEndSet(*this); }

		return true;
	}

	bool decodeUnknown(ReaderType &reader, DataReader<ByteOrder::Network> &r, uint8_t t) {
		auto size = decodeSize(r);
		r += size;

		if constexpr (Traits::onCustom) { reader.onCustom(*this, t, DataReader<ByteOrder::Network>(r.data(), size)); }
		return true;
	}

	bool decodeAny(ReaderType &reader, DataReader<ByteOrder::Network> &r) {
		auto size = decodeSize(r);
		if (size == 0) {
			return false;
		}

		DataReader<ByteOrder::Network> nextR(r.data(), size);
		if (decodeValue(reader, nextR)) {
			r += size;
			return true;
		}
		return false;
	}

	bool decodeOid(ReaderType &reader, DataReader<ByteOrder::Network> &r) {
		auto size = decodeSize(r);
		if (size == 0) {
			return false;
		}

		StringStream str;

		DataReader<ByteOrder::Network> nextR(r.data(), size);

		auto first = nextR.readUnsigned();

		str << int(first / 40) << "." << int(first % 40);

		uint32_t accum = 0;
		while (!nextR.empty()) {
			auto value = nextR.readUnsigned();
			if (value & 0x80) {
				accum = accum << 7 | (value & (~0x80));
			} else {
				str << "." << (accum << 7 | value);
				accum = 0;
			}
		}

		r += size;

		if constexpr (Traits::onOid) {
			reader.onOid(*this, str.str());
		}

		return true;
	}

	bool decodeInteger(ReaderType &reader, DataReader<ByteOrder::Network> &r) {
		auto size = decodeSize(r);
		if (size == 0) {
			return false;
		}

		if constexpr (Traits::onInteger) {
			switch(size) {
			case 1: reader.onInteger(*this, reinterpretValue<int8_t>(r.readUnsigned())); break;
			case 2: reader.onInteger(*this, reinterpretValue<int16_t>(r.readUnsigned16())); break;
			case 3: {
				auto val = r.readUnsigned24();
				if (val <= 0x7FFFFF) {
					reader.onInteger(*this, val);
				} else {
					reader.onInteger(*this, val - 0x1000000);
				}
			}
				break;
			case 4: reader.onInteger(*this, reinterpretValue<int32_t>(r.readUnsigned32())); break;
			case 8: reader.onInteger(*this, reinterpretValue<int64_t>(r.readUnsigned64())); break;
			default:
				if (size >= 8) {
					return false;
				} else {
					uint64_t accum = 0;
					for (uint32_t i = 0; i < size; ++ i) {
						accum = (accum << 8) | r.readUnsigned();
					}
					uint64_t base = 1ULL << (size * 8 - 1);
					if (accum > base) {
						reader.onInteger(*this, accum - (1ULL << (size * 8)));
					} else {
						reader.onInteger(*this, accum);
					}
				}

				break;
			}
		} else {
			r += size;
		}

		return true;
	}

	bool decodeBoolean(ReaderType &reader, DataReader<ByteOrder::Network> &r) {
		auto size = decodeSize(r);
		if (size == 1) {
			auto value = r.readUnsigned();
			if constexpr (Traits::onBoolean) { reader.onBoolean(*this, value != 0); }
			return true;
		}

		return false;
	}

	bool decodeOctetString(ReaderType &reader, DataReader<ByteOrder::Network> &r) {
		auto size = decodeSize(r);
		if (size > 0) {
			if constexpr (Traits::onBytes) { reader.onBytes(*this, DataReader<ByteOrder::Network>(r.data(), size)); }
			r += size;
			return true;
		}

		return true;
	}

	bool decodeString(ReaderType &reader, DataReader<ByteOrder::Network> &r, Type t) {
		auto size = decodeSize(r);
		if constexpr (Traits::onString) { reader.onString(*this, DataReader<ByteOrder::Network>(r.data(), size), t); }
		r += size;
		return true;
	}

	bool decodeBitString(ReaderType &reader, DataReader<ByteOrder::Network> &r, bool constructed) {
		auto size = decodeSize(r);

		if (!constructed) {
			if (size > 1) {
				auto unused = r.readUnsigned();

				if (unused > 0 && unused < 8) {
					Bytes b; b.resize(size  - 1);
					memcpy(b.data(), r.data(), size - 1);

					uint8_t mask = 0xFF << unused;
					b.back() = b.back() & mask;

					if constexpr (Traits::onBytes) { reader.onBytes(*this, DataReader<ByteOrder::Network>(b.data(), b.size())); }
					r += size - 1;
					return true;
				} else if (unused == 0) {
					if constexpr (Traits::onBytes) { reader.onBytes(*this, DataReader<ByteOrder::Network>(r.data(), size - 1)); }
					r += size - 1;
					return true;
				}
			}
			return false;
		} else {
			r += size;
			return true;
		}
	}

	bool decode(ReaderType &reader, const Bytes &source) {
		return decode(reader, DataReader<ByteOrder::Network>(source));
	}

	bool decode(ReaderType &reader, const DataReader<ByteOrder::Network> &source) {
		DataReader<ByteOrder::Network> r(source);

		while (!r.empty()) {
			if (!decodeValue(reader, r)) {
				return false;
			}
		}

		return true;
	}
};

struct Asn1ItemReader {
	using Decoder = Asn1Decoder<Asn1ItemReader>;

	Asn1ItemReader(const DataReader<ByteOrder::Network> &source, data::Value &val) {
		result = &val;
		Decoder dec;
		dec.decode(*this, source);
	}

	void onOid(Decoder &, const String &str) {
		result->setString(str);
	}

	void onNull(Decoder &) {
		result->setNull();
	}

	void onInteger(Decoder &, int64_t val) {
		result->setInteger(val);
	}

	void onBoolean(Decoder &, bool val) {
		result->setBool(val);
	}

	void onBytes(Decoder &, const DataReader<ByteOrder::Network> &r) {
		result->setBytes(Bytes(r.data(), r.data() + r.size()));
	}

	void onString(Decoder &, const DataReader<ByteOrder::Network> &r, Decoder::Type t) {
		if (r.size() > 0) {
			result->setString(String((char*)r.data(), r.size()));
		}
	}

	data::Value * result;
};

struct Asn1PayloadReader {
	using Decoder = Asn1Decoder<Asn1PayloadReader>;

	enum ReceiptType : int32_t {
		BundleIdentifier = 2, // UTF8STRING
		AppVersion = 3, // UTF8STRING
		OpaqueValue = 4,// An opaque value used, with other data, to compute the SHA-1 hash during validation.
		ReceiptHash = 5, // A SHA-1 hash, used to validate the receipt.
		InAppReceipt = 17, // In-App Purchase Receipt
		OriginalAppVersion = 19,
		ReceiptCreationDate = 12, // IA5STRING
		ReceiptExpirationDate = 21, // IA5STRING

		InAppQuantity = 1701, // INTEGER
		InAppProductIdentifier = 1702, // UTF8STRING
		InAppTransactionIdentifier = 1703, // UTF8STRING
		InAppOriginalTransactionIdentifier = 1705, // UTF8STRING
		InAppPurchaseDate = 1704, // IA5STRING, interpreted as an RFC 3339 date
		InAppOriginalPurchaseDate = 1706, //IA5STRING, interpreted as an RFC 3339 date
		InAppSubscriptionExpirationDate = 1708, // IA5STRING, interpreted as an RFC 3339 date
		InAppCancellationDate = 1712, // IA5STRING, interpreted as an RFC 3339 date
		InAppWebOrderLineItemID = 1711, // INTEGER
	};

	enum class State {
		None,
		Item,
	};

	Asn1PayloadReader(const DataReader<ByteOrder::Network> &source) {
		Decoder dec;
		dec.decode(*this, source);
	}

	void onBeginSequence(Decoder &) {
		state = State::Item;
		currentItem = 0;
	}
	void onEndSequence(Decoder &) {
		state = State::None;
		currentItem = 0;
	}

	void onInteger(Decoder &, int64_t val) {
		if (state == State::Item && currentItem == 0) {
			currentItem = val;
		}
	}

	void onBytes(Decoder &, const DataReader<ByteOrder::Network> &r) {
		if (state == State::Item && currentItem != 0) {
			switch (currentItem) {
			case BundleIdentifier:
				result.setValue(readItem(r), "bundle_id");
				break;
			case AppVersion:
				result.setValue(readItem(r), "application_version");
				break;
			case OpaqueValue:
				result.setBytes(Bytes(r.data(), r.data() + r.size()), "opaque");
				break;
			case ReceiptHash:
				result.setBytes(Bytes(r.data(), r.data() + r.size()), "receipt_hash");
				break;
			case InAppReceipt:
				if (!result.hasValue("in_app")) {
					result.setValue(data::Value(data::Value::Type::ARRAY), "in_app");
				}
				result.getValue("in_app").addValue(readSubItems(r));
				break;
			case OriginalAppVersion:
				result.setValue(readItem(r), "original_application_version");
				break;
			case ReceiptCreationDate:
				result.setValue(readItem(r), "creation_date");
				if (result.isString("creation_date")) {
					result.setInteger(readTime(result.getString("creation_date")), "creation_date_ms");
				}
				break;
			case ReceiptExpirationDate:
				result.setValue(readItem(r), "expiration_date");
				if (result.isString("expiration_date")) {
					result.setInteger(readTime(result.getString("expiration_date")), "expiration_date_ms");
				}
				break;
			case InAppQuantity:
				result.setValue(readItem(r), "quantity");
				break;
			case InAppProductIdentifier:
				result.setValue(readItem(r), "product_id");
				break;
			case InAppTransactionIdentifier:
				result.setValue(readItem(r), "transaction_id");
				break;
			case InAppOriginalTransactionIdentifier:
				result.setValue(readItem(r), "original_transaction_id");
				break;
			case InAppPurchaseDate:
				result.setValue(readItem(r), "purchase_date");
				if (result.isString("purchase_date")) {
					result.setInteger(readTime(result.getString("purchase_date")), "purchase_date_ms");
				}
				break;
			case InAppOriginalPurchaseDate:
				result.setValue(readItem(r), "original_purchase_date");
				if (result.isString("original_purchase_date")) {
					result.setInteger(readTime(result.getString("original_purchase_date")), "original_purchase_date_ms");
				}
				break;
			case InAppSubscriptionExpirationDate:
				result.setValue(readItem(r), "expires_date");
				if (result.isString("expires_date")) {
					result.setInteger(readTime(result.getString("expires_date")), "expires_date_ms");
				}
				break;
			case InAppCancellationDate:
				result.setValue(readItem(r), "cancellation_date");
				if (result.isString("cancellation_date")) {
					result.setInteger(readTime(result.getString("cancellation_date")), "cancellation_date_ms");
				}
				break;
			case InAppWebOrderLineItemID:
				result.setValue(readItem(r), "web_order_line_item_id");
				break;
			}
		}
	}

	int64_t readTime(const String &str) {
		std::istringstream infile(str);
		std::chrono::system_clock::time_point tp;
		infile >> tp;
		auto s = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
		return s;
	}

	data::Value readItem(const DataReader<ByteOrder::Network> &r) {
		data::Value ret;
		Asn1ItemReader reader(r, ret);
		return ret;
	}

	data::Value readSubItems(const DataReader<ByteOrder::Network> &r) {
		Asn1PayloadReader reader(r);
		return reader.result;
	}

	State state = State::None;
	int64_t currentItem = 0;
	data::Value result;
};

struct Asn1Reader {
	using Decoder = Asn1Decoder<Asn1Reader>;

	enum class State {
		None,
		SignedData,
		Payload,
	};

	Asn1Reader(const Bytes &source) {
		Decoder dec;
		dec.decode(*this, source);
	}

	Asn1Reader(const DataReader<ByteOrder::Network> &source) {
		Decoder dec;
		dec.decode(*this, source);
	}

	void onBeginSet(Decoder &) {
		if (verbose) {
			doIndent();
			std::cout << "BeginSet" << "\n";
			++ indent;
		}
	}
	void onEndSet(Decoder &) {
		if (verbose) {
			-- indent;
			doIndent();
			std::cout << "EndSet" << "\n";
		}
	}

	void onBeginSequence(Decoder &) {
		if (state == State::SignedData) {
			++ sequenceDepth;
		}
		if (verbose) {
			doIndent();
			std::cout << "BeginSequence" << '\n';
			++ indent;
		}
	}
	void onEndSequence(Decoder &) {
		switch (state) {
		case State::SignedData:
			if (sequenceDepth == 1) {
				state = State::None;
				sequenceDepth = 0;
			} else {
				-- sequenceDepth;
			}
			break;
		default: break;
		}

		if (verbose) {
			-- indent;
			doIndent();
			std::cout << "EndSequence" << '\n';
		}
	}

	void onOid(Decoder &, const String &str) {
		if (str == "1.2.840.113549.1.7.2") {
			state = State::SignedData;
			sequenceDepth = 1;
		} else if (str == "1.2.840.113549.1.7.1") {
			state = State::Payload;
		}

		if (verbose) {
			doIndent();
			std::cout << "OID: " << str << '\n';
		}
	}

	void onNull(Decoder &) {
		if (state == State::Payload) {
			state = State::SignedData;
		}
		if (verbose) {
			doIndent();
			std::cout << "Null" << '\n';
		}
	}

	void onInteger(Decoder &, int64_t val) {
		if (verbose) {
			doIndent();
			std::cout << "Integer: " << val << '\n';
		}
	}

	void onBoolean(Decoder &, bool val) {
		if (verbose) {
			doIndent();
			std::cout << "Boolean: ";
			if (val) {
				std::cout << "true";
			} else {
				std::cout << "false";
			}
			std::cout << '\n';
		}
	}

	void onBytes(Decoder &, const DataReader<ByteOrder::Network> &r) {
		if (verbose) {
			doIndent();
			std::cout << "Bytes: (" << r.size() << ") " << base16::encode(CoderSource(r)) << '\n';
		}

		if (state == State::Payload) {
			Asn1PayloadReader p(r);
			payload = std::move(p.result);
			state = State::SignedData;
		}
	}

	void onString(Decoder &, const DataReader<ByteOrder::Network> &r, Decoder::Type t) {
		if (verbose) {
			doIndent();
			std::cout << "String: " << String((char*)r.data(), r.size()) << '\n';
		}
	}

	void doIndent() {
		for (size_t i = 0; i < indent; ++ i) {
			std::cout << '\t';
		}
	}

	State state = State::None;
	size_t indent = 0;
	size_t sequenceDepth = 0;
	bool verbose = false;

	data::Value payload;
};

data::Value validateReceipt(const DataReader<ByteOrder::Network> &data) {
	if (data.size() > 0) {
		Asn1Reader reader(data);
		return reader.payload;
	}
	return data::Value();
}

data::Value validateReceipt(const Bytes &data) {
	return validateReceipt(DataReader<ByteOrder::Network>(data));
}

data::Value validateReceipt(const String &path) {
	return validateReceipt(stappler::filesystem::readIntoMemory(path));
}

NS_SP_EXT_END(platform)

#endif
