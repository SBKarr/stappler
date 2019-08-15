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

#ifndef COMMON_DATA_SPDATA_H_
#define COMMON_DATA_SPDATA_H_

#include "SPDataEncode.h"
#include "SPDataDecode.h"

NS_SP_EXT_BEGIN(data)

using DataCallback = Function<void(data::Value &&)>;

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
		const Function<int (Value &ret, const StringView &str, int argc, const char * argv[])> &stringCallback);

Value parseCommandLineOptions(int argc, const char16_t * argv[],
		const Function<int (Value &ret, char c, const char *str)> &switchCallback,
		const Function<int (Value &ret, const StringView &str, int argc, const char * argv[])> &stringCallback);

class Transform : AllocBase {
public:
	template <typename ... Args>
	explicit Transform(Args && ...args) {
		init(std::forward<Args>(args)...);
	}

	Transform() noexcept;
	Transform(const Transform &) noexcept;
	Transform(Transform &&) noexcept;

	Transform & operator=(const Transform &) noexcept;
	Transform & operator=(Transform &&) noexcept;

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

#endif // COMMON_DATA_SPDATA_H_
