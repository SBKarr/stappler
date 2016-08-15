
#ifndef __stappler__SPData__
#define __stappler__SPData__

#include "SPDataValue.h"

NS_SP_EXT_BEGIN(data)

using DataCallback = Function<void(data::Value &)>;

struct EncodeFormat {
	static int EncodeStreamIndex;

	enum Format {
		Json				= 0b0000, // Raw JSON data, with no whitespace
		Pretty				= 0b0001, // Pretty-printed JSON data
		Cbor				= 0b0010, // CBOR data (http://cbor.io/, http://tools.ietf.org/html/rfc7049)
		DefaultFormat		= 0b0011,
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

	explicit EncodeFormat(int flag)
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

Value readFile(const String &filename, const String &key = "");
Value read(const String &string, const String &key = "");
Value read(const Bytes &vec, const String &key = "");

Bytes write(const data::Value &, EncodeFormat fmt = EncodeFormat());
bool write(std::ostream &, const data::Value &, EncodeFormat fmt = EncodeFormat());
bool save(const data::Value &, const String &file, EncodeFormat fmt = EncodeFormat());

String toString(const data::Value &, bool pretty = false);

// command line options parsing
//
// arguments, prefixed with '-' resolved as char switches
// 'switchCallback' used to process this chars
// 'char c' - current char,
// 'const char *str' - string in which this char was found, str[0] = c;
// return value - number of processed chars (usually - 1)
// For string '-name' switchCallback will be called 4 times with each char in string,
// but you can process whole string in first call, then return 4 from callback
//
// arguments, prefixed with '--' resolved as string options
// 'stringCallback' used to process this strings
// 'const String &str' - current parsed string
// 'int argc' - number of strings in argv
// 'const char * argv[]' - remaining strings to parse
// return value - number of parsed strings (usually 1)
//
// Other strings will be added into array 'args' in return value

Value parseCommandLineOptions(int argc, const char * argv[],
		const Function<int (Value &ret, char c, const char *str)> &switchCallback,
		const Function<int (Value &ret, const String &str, int argc, const char * argv[])> &stringCallback);

Value parseCommandLineOptions(int argc, const char16_t * argv[],
		const Function<int (Value &ret, char c, const char *str)> &switchCallback,
		const Function<int (Value &ret, const String &str, int argc, const char * argv[])> &stringCallback);

class Transform : AllocBase {
public:
	template <typename ... Args>
	explicit Transform(Args && ...args) {
		init(std::forward<Args>(args)...);
	}

	Transform();
	Transform(const Transform &);
	Transform(Transform &&);

	Transform & operator=(const Transform &);
	Transform & operator=(Transform &&);

	data::Value &transform(data::Value &) const;
	Transform reverse() const;

	data::Value data() const;

	bool empty() const;

	String transformKey(const String &) const;

protected:
	enum class Mode {
		Map,
		Sub
	};

	void performTransform(data::Value &value, Vector<data::Value *> &stack, const Pair<String, Vector<String>> &it) const;
	void performTransform(data::Value &value, Vector<data::Value *> &stack, const Pair<String, Transform> &it) const;
	void makeTransform(data::Value &, Vector<data::Value *> &stack) const;


	void performReverse(Transform &, Vector<Pair<const String *, Transform *>> &, const Pair<String, Vector<String>> &it) const;
	void performReverse(Transform &, Vector<Pair<const String *, Transform *>> &, const Pair<String, Transform> &it) const;
	void makeReverse(Transform &, Vector<Pair<const String *, Transform *>> &stack) const;

	void emplace(Pair<String, String> &&p) {
		order.emplace_back(pair(p.first, Mode::Map));
		map.emplace(std::move(p.first), Vector<String>{p.second});
	}

	void emplace(Pair<String, Vector<String>> &&p) {
		order.emplace_back(pair(p.first, Mode::Map));
		map.emplace(std::move(p.first), std::move(p.second));
	}

	void emplace(Pair<String, Transform> &&p) {
		order.emplace_back(pair(p.first, Mode::Sub));
		subtransforms.emplace(std::move(p.first), std::move(p.second));
	}

	void init() { }

	template <typename T, typename ... Args>
	void init(T && t, Args && ...args) {
		emplace(std::forward<T>(t));
		init(std::forward<Args>(args)...);
	}

	Vector<Pair<String, Mode>> order;
	Map<String, Vector<String>> map;
	Map<String, Transform> subtransforms;
};

struct TransformPair {
	Transform input;
	Transform output;

	TransformPair() { }

	TransformPair(const data::Transform &i) : input(i), output(i.reverse()) {}

	TransformPair(const data::Transform &i, const data::Transform &o)
	: input(i), output(o) {
		if (input.empty() && !output.empty()) {
			input = output.reverse();
		}

		if (!input.empty() && output.empty()) {
			output = input.reverse();
		}
	}
};

using TransformMap = Map<String, TransformPair>;

NS_SP_EXT_END(data)

#include "SPDataUtils.hpp"

#endif
