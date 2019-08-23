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

#include "SPError.h"

#if DEBUG
#include <typeinfo>
#if LINUX
#include <cxxabi.h>
#endif
#endif

#include "base/ccMacros.h"
#include "platform/CCPlatformMacros.h"
#include "base/CCDirector.h"
#include "2d/CCNode.h"
#include "SPThreadManager.h"
#include "SPThread.h"

#include "SPString.h"
#include "SPData.h"

NS_SP_BEGIN

namespace node {
	data::Value dumpNode(cocos2d::Node *node, int depth) {
		data::Value val;
		val.setBool(node->isVisible(), "visible");
		val.setInteger(node->getOpacity(), "opacity");
		val.setString(toString("x: ", node->getPositionX(), ", y: ", node->getPositionY(),
				", width: ", node->getContentSize().width, ", height: ", node->getContentSize().height), "rect");
#if DEBUG
#if LINUX
		int status;
		auto realname = abi::__cxa_demangle(typeid(*node).name(), 0, 0, &status);
		val.setString(realname, "type");
		free(realname);
#else
		val.setString(typeid(*node).name(), "type");
#endif
#endif

		if (depth != 0) {
			data::Value childData;
			auto &childs = node->getChildren();
			for (auto &n : childs) {
				childData.addValue(dumpNode(n, depth - 1));
			}

			if (childData) {
				val.setValue(std::move(childData), "childs");
			}
		}

		return val;
	}

	void dump(cocos2d::Node *node, int depth) {
		auto val = dumpNode(node, depth);
		log::text("Dump", data::toString(val));
	}
}

NS_SP_END
