/*
 * SPDebug.cpp
 *
 *  Created on: 21 июля 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPDefine.h"
#include "SPFilesystem.h"

#ifdef LINUX

#include <sstream>
#include <execinfo.h>
#include <cxxabi.h>

NS_SP_BEGIN

void PrintBacktrace(int len, cocos2d::Ref *ref) {
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
    	//auto first = str.find('(');
    	//auto second = str.rfind('+');
    	//str = str.substr(first + 1, second - first - 1);

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
void PrintBacktrace(int len, cocos2d::Ref *ref) {

}
NS_SP_END

#endif

