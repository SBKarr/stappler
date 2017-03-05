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

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <stdint.h>

/* Generator for color lookup table with material colors
 * (https://material.io/guidelines/style/color.html#color-color-palette)
 *
 * Usage example can be found in layout/types/SLColor.cpp
 * Takes approximate 8 KiB in application binary
 */

using namespace std;

namespace hash_ns {

// see https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1_hash
// parameters from http://www.boost.org/doc/libs/1_38_0/libs/unordered/examples/fnv1.hpp

template <class T> constexpr  T _fnv_offset_basis();
template <class T> constexpr  T _fnv_prime();

template <> constexpr uint64_t _fnv_offset_basis<uint64_t>() { return uint64_t(14695981039346656037llu); }
template <> constexpr uint64_t _fnv_prime<uint64_t>() { return uint64_t(1099511628211llu); }

template <> constexpr uint32_t _fnv_offset_basis<uint32_t>() { return uint32_t(2166136261llu); }
template <> constexpr uint32_t _fnv_prime<uint32_t>() { return uint32_t(16777619llu); }

template <class T>
constexpr T _fnv1(const uint8_t* ptr, size_t len) {
	T hash = _fnv_offset_basis<T>();
	for (size_t i = 0; i < len; i++) {
	     hash *= _fnv_prime<T>();
	     hash ^= ptr[i];
	}
	return hash;
}

constexpr uint32_t hash32(const char* str, size_t len) {
    return _fnv1<uint32_t>((uint8_t*)str, len);
}

constexpr uint64_t hash64(const char* str, size_t len) {
    return _fnv1<uint64_t>((uint8_t*)str, len);
}

}

// used for naming/hashing (like "MyTag"_tag)
constexpr uint32_t operator"" _hash ( const char* str, size_t len) { return hash_ns::hash32(str, len); }

#define MD_COLOR_TABLE_COMPONENT(Name, Id, Value, Index, Group) \
		table.emplace_back(ColorDataTable{0x ## Value, uint16_t(Group * 16 + Index), #Name #Id ## _hash, #Name #Id}); \

#define MD_COLOR_TABLE_BASE_DEFINE(Name, Group, b50, b100, b200, b300, b400, b500, b600, b700, b800, b900 ) \
	MD_COLOR_TABLE_COMPONENT(Name, 50, b50, 0, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, 100, b100, 1, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, 200, b200, 2, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, 300, b300, 3, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, 400, b400, 4, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, 500, b500, 5, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, 600, b600, 6, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, 700, b700, 7, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, 800, b800, 8, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, 900, b900, 9, Group)

#define MD_COLOR_TABLE_ACCENT_DEFINE(Name, Group, a100, a200, a400, a700 ) \
	MD_COLOR_TABLE_COMPONENT(Name, A100, a100, 10, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, A200, a200, 11, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, A400, a400, 12, Group) \
	MD_COLOR_TABLE_COMPONENT(Name, A700, a700, 13, Group)

#define MD_COLOR_TABLE_DEFINE(Name, Group, b50, b100, b200, b300, b400, b500, b600, b700, b800, b900, a100, a200, a400, a700) \
	MD_COLOR_TABLE_BASE_DEFINE(Name, Group, b50, b100, b200, b300, b400, b500, b600, b700, b800, b900) \
	MD_COLOR_TABLE_ACCENT_DEFINE(Name, Group, a100, a200, a400, a700)

struct ColorData {
	string str;
	uint32_t hash;
	uint32_t value;
	uint16_t index;
};

struct ColorDataTable {
	uint32_t value;
	uint16_t index;
	uint32_t hash;
	char str[16];
};

struct ColorValueIdx {
	uint32_t value;
	uint8_t idx;
};

struct ColorIndexIdx {
	uint16_t index;
	uint8_t idx;
};

struct ColorNameIdx {
	uint32_t hash;
	uint8_t idx;
};

int main() {
	vector<ColorDataTable> table;

	vector<ColorValueIdx> values;
	vector<ColorIndexIdx> indexes;
	vector<ColorNameIdx> names;

	MD_COLOR_TABLE_DEFINE(Red, 0, ffebee, ffcdd2, ef9a9a, e57373, ef5350, f44336, e53935, d32f2f, c62828, b71c1c, ff8a80, ff5252, ff1744, d50000 );
	MD_COLOR_TABLE_DEFINE(Pink, 1, fce4ec, f8bbd0, f48fb1, f06292, ec407a, e91e63, d81b60, c2185b, ad1457, 880e4f, ff80ab, ff4081, f50057, c51162 );
	MD_COLOR_TABLE_DEFINE(Purple, 2, f3e5f5, e1bee7, ce93d8, ba68c8, ab47bc, 9c27b0, 8e24aa, 7b1fa2, 6a1b9a, 4a148c, ea80fc, e040fb, d500f9, aa00ff );
	MD_COLOR_TABLE_DEFINE(DeepPurple, 3, ede7f6, d1c4e9, b39ddb, 9575cd, 7e57c2, 673ab7, 5e35b1, 512da8, 4527a0, 311b92, b388ff, 7c4dff, 651fff, 6200ea );
	MD_COLOR_TABLE_DEFINE(Indigo, 4, e8eaf6, c5cae9, 9fa8da, 7986cb, 5c6bc0, 3f51b5, 3949ab, 303f9f, 283593, 1a237e, 8c9eff, 536dfe, 3d5afe, 304ffe );
	MD_COLOR_TABLE_DEFINE(Blue, 5, e3f2fd, bbdefb, 90caf9, 64b5f6, 42a5f5, 2196f3, 1e88e5, 1976d2, 1565c0, 0d47a1, 82b1ff, 448aff, 2979ff, 2962ff );
	MD_COLOR_TABLE_DEFINE(LightBlue, 6, e1f5fe, b3e5fc, 81d4fa, 4fc3f7, 29b6f6, 03a9f4, 039be5, 0288d1, 0277bd, 01579b, 80d8ff, 40c4ff, 00b0ff, 0091ea );
	MD_COLOR_TABLE_DEFINE(Cyan, 7, e0f7fa, b2ebf2, 80deea, 4dd0e1, 26c6da, 00bcd4, 00acc1, 0097a7, 00838f, 006064, 84ffff, 18ffff, 00e5ff, 00b8d4 );
	MD_COLOR_TABLE_DEFINE(Teal, 8, e0f2f1, b2dfdb, 80cbc4, 4db6ac, 26a69a, 009688, 00897b, 00796b, 00695c, 004d40, a7ffeb, 64ffda, 1de9b6, 00bfa5 );
	MD_COLOR_TABLE_DEFINE(Green, 9, e8f5e9, c8e6c9, a5d6a7, 81c784, 66bb6a, 4caf50, 43a047, 388e3c, 2e7d32, 1b5e20, b9f6ca, 69f0ae, 00e676, 00c853 );
	MD_COLOR_TABLE_DEFINE(LightGreen, 10, f1f8e9, dcedc8, c5e1a5, aed581, 9ccc65, 8bc34a, 7cb342, 689f38, 558b2f, 33691e, ccff90, b2ff59, 76ff03, 64dd17 );
	MD_COLOR_TABLE_DEFINE(Lime, 11, f9fbe7, f0f4c3, e6ee9c, dce775, d4e157, cddc39, c0ca33, afb42b, 9e9d24, 827717, f4ff81, eeff41, c6ff00, aeea00 );
	MD_COLOR_TABLE_DEFINE(Yellow, 12, fffde7, fff9c4, fff59d, fff176, ffee58, ffeb3b, fdd835, fbc02d, f9a825, f57f17, ffff8d, ffff00, ffea00, ffd600 );
	MD_COLOR_TABLE_DEFINE(Amber, 13, fff8e1, ffecb3, ffe082, ffd54f, ffca28, ffc107, ffb300, ffa000, ff8f00, ff6f00, ffe57f, ffd740, ffc400, ffab00 );
	MD_COLOR_TABLE_DEFINE(Orange, 14, fff3e0, ffe0b2, ffcc80, ffb74d, ffa726, ff9800, fb8c00, f57c00, ef6c00, e65100, ffd180, ffab40, ff9100, ff6d00 );
	MD_COLOR_TABLE_DEFINE(DeepOrange, 15, fbe9e7, ffccbc, ffab91, ff8a65, ff7043, ff5722, f4511e, e64a19, d84315, bf360c, ff9e80, ff6e40, ff3d00, dd2c00 );
	MD_COLOR_TABLE_BASE_DEFINE(Brown, 16, efebe9, d7ccc8, bcaaa4, a1887f, 8d6e63, 795548, 6d4c41, 5d4037, 4e342e, 3e2723 );
	MD_COLOR_TABLE_BASE_DEFINE(Grey, 17, fafafa, f5f5f5, eeeeee, e0e0e0, bdbdbd, 9e9e9e, 757575, 616161, 424242, 212121 );
	MD_COLOR_TABLE_BASE_DEFINE(BlueGrey, 18, eceff1, cfd8dc, b0bec5, 90a4ae, 78909c, 607d8b, 546e7a, 455a64, 37474f, 263238 );

	table.emplace_back(ColorDataTable{0xFFFFFF, (uint16_t)19 * 16 + 0, "White"_hash, "White"});
	table.emplace_back(ColorDataTable{0x000000, (uint16_t)19 * 16 + 1, "Black"_hash, "Black"});

	cout << "colors: " << table.size() << " " << sizeof(ColorDataTable)
			<< " " << sizeof(ColorValueIdx) << " " << sizeof(ColorIndexIdx) << " " << sizeof(ColorNameIdx)
			<< " " << sizeof(ColorDataTable) * table.size()
			<< " " << sizeof(ColorValueIdx) * table.size()
			<< " " << sizeof(ColorIndexIdx) * table.size()
			<< " " << sizeof(ColorNameIdx) * table.size()
			<< " " << (sizeof(ColorDataTable) + sizeof(ColorValueIdx) + sizeof(ColorIndexIdx) + sizeof(ColorNameIdx)) * table.size()
			<< endl;

	uint8_t i = 0;

	cout << "{" << endl;
	for (auto &it : table) {
		values.emplace_back(ColorValueIdx{it.value, i});
		indexes.emplace_back(ColorIndexIdx{it.index, i});
		names.emplace_back(ColorNameIdx{it.hash, i});
		++ i;

		cout << "\t{ 0x" << std::setw(6) << setfill('0') << hex << it.value
				<< ", 0x" << std::setw(3) << setfill('0') << hex << it.index
				<< ", 0x" << std::setw(8) << setfill('0') << hex << it.hash
				<< ", \"" << it.str << "\" }," << endl;
	}
	cout << "}" << endl << endl;

	sort(values.begin(), values.end(), [] (const ColorValueIdx &l, const ColorValueIdx &r) -> bool {
		return l.value < r.value;
	});
	sort(indexes.begin(), indexes.end(), [] (const ColorIndexIdx &l, const ColorIndexIdx &r) -> bool {
		return l.index < r.index;
	});
	sort(names.begin(), names.end(), [] (const ColorNameIdx &l, const ColorNameIdx &r) -> bool {
		return l.hash < r.hash;
	});

	uint16_t accum = 0;

	cout << "{" << endl;
	for (auto &it : values) {
		if (accum == 0) {
			cout << "\t";
		}

		cout << "{ 0x" << std::setw(6) << setfill('0') << hex << it.value
				<< ", 0x" << std::setw(2) << setfill('0') << hex << int(it.idx) << " }, ";

		accum ++;
		if (accum >= 8) {
			cout << endl;
			accum = 0;
		}
	}
	cout << "}" << endl << endl;


	cout << "{" << endl;
	for (auto &it : indexes) {
		if (accum == 0) {
			cout << "\t";
		}

		cout << "{ 0x" << std::setw(3) << setfill('0') << hex << it.index
				<< ", 0x" << std::setw(2) << setfill('0') << hex << int(it.idx) << " }, ";

		accum ++;
		if (accum >= 8) {
			cout << endl;
			accum = 0;
		}
	}
	cout << "}" << endl << endl;


	cout << "{" << endl;
	for (auto &it : names) {
		if (accum == 0) {
			cout << "\t";
		}

		cout << "{ 0x" << std::setw(8) << setfill('0') << hex << it.hash
				<< ", 0x" << std::setw(2) << setfill('0') << hex << int(it.idx) << " }, ";

		accum ++;
		if (accum >= 8) {
			cout << endl;
			accum = 0;
		}
	}
	cout << "}" << endl << endl;

	return 0;
}
