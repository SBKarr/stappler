// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDefine.h"
#include "SPDefine.h"
#include "SPFilesystem.h"

#ifdef LINUX

#include <sstream>
#include <execinfo.h>
#include <cxxabi.h>

NS_SP_BEGIN

void PrintBacktrace(int len, Ref *ref) {
    void *bt[len + 2];
    char **bt_syms;
    int bt_size;

    bt_size = backtrace(bt, len + 2);
    bt_syms = backtrace_symbols(bt, bt_size);

    std::stringstream stream;
    if (ref) {
    	stream << "Ref: " << ref << " (refcount: " << ref->getReferenceCount() << ")";
    }
    stream << "\n";

    for (int i = 2; i < bt_size; i++) {
    	auto str = filepath::replace(bt_syms[i], filesystem::currentDir(), "/");
    	auto first = str.find('(');
    	auto second = str.rfind('+');
    	str = str.substr(first + 1, second - first - 1);

    	int status = 0;
    	auto ptr = abi::__cxa_demangle (str.c_str(), nullptr, nullptr, &status);

    	if (ptr) {
        	stream << "\t[" << i - 2 << "] " << std::string(ptr) << "\n";
        	free(ptr);
    	} else {
    		stream << "\t[" << i - 2 << "] " << bt_syms[i] << "\n";
    	}
    }

	log::text("Backtrace", stream.str());

    free(bt_syms);
}

NS_SP_END

#else

NS_SP_BEGIN
void PrintBacktrace(int len, Ref *ref) {

}
NS_SP_END

#endif

