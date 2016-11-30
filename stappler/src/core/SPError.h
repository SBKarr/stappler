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

#ifndef __stappler__SPError__
#define __stappler__SPError__

#include "SPDefine.h"
#include "SPData.h"
#include "SPEventHeader.h"

#include "2d/CCNode.h"

NS_SP_BEGIN

class LogStream {
public:
	LogStream(const std::string &tag = "") : tag(tag) { }
	~LogStream() {
		auto str = stream.str();
		if (!str.empty()) {
			if (tag.empty()) {
				log::text("Stream", str.c_str());
			} else {
				log::text(tag.c_str(), str.c_str());
			}
		}
		stream.clear();
	}

	struct Push { };

	LogStream& operator<<(const Rect & obj) {
		stream << "Rect(x:" << obj.origin.x << " y:" << obj.origin.y
				<< " width:" << obj.size.width << " height:" << obj.size.height << ");";
		return *this;
	}

	LogStream& operator<<(const Size & obj) {
		stream << "Size(width:" << obj.width << " height:" << obj.height << ");";
		return *this;
	}

	LogStream& operator<<(const Vec2 & obj) {
		stream << "Vec2(x:" << obj.x << " y:" << obj.y << ");";
		return *this;
	}

	LogStream& operator<<(const Vec3 & obj) {
		stream << "Vec3(x:" << obj.x << " y:" << obj.y << " z:" << obj.z << ");";
		return *this;
	}

	LogStream& operator<<(const Color3B & obj) {
		stream << "Color3B(r:" << (int)obj.r << " g:" << (int)obj.g << " b:" << (int)obj.b << ");";
		return *this;
	}

	LogStream& operator<<(const Color4B & obj) {
		stream << "Color4B(r:" << (int)obj.r << " g:" << (int)obj.g << " b:" << (int)obj.b << " a:" << (int)obj.a << ");";
		return *this;
	}

	LogStream& operator<<(const Padding & obj) {
		stream << "Padding(top:" << obj.top << " right:" << obj.right
				<< " bottom:" << obj.bottom << " left:" << obj.left << ");";
		return *this;
	}

	LogStream& operator<<(const Push & obj) {
		if (tag.empty()) {
			log::text("Stream", stream.str().c_str());
		} else {
			log::text(tag.c_str(), stream.str().c_str());
		}
		stream.clear();
		return *this;
	}

	LogStream& operator<<(cocos2d::Node * const & node) {
		(*this) << "Node {\n"
				<< "\tPosition: " << node->getPosition() << "\n"
				<< "\tAnchorPoint: " << node->getAnchorPoint() << "\n"
				<< "\tContentSize: " << node->getContentSize() << "\n"
				<< "\tBoundingBox: " << node->getBoundingBox() << "\n"
				<< "\tColor: " << node->getColor() << " (" << node->getDisplayedColor() << ")" << "\n"
				<< "\tOpacity: " << (int)node->getOpacity() << " (" << (int)node->getDisplayedOpacity() << ")" << "\n"
				<< "\tVisible: " << (node->isVisible()?"true":"false") << "\n"
				<< "};";
		return *this;
	}

	template <class T>
	LogStream& operator<<(const T &t) {
		stream << t;
		return *this;
	}
protected:
	std::string tag;
	std::stringstream stream;
};

NS_SP_END

#endif /* defined(__stappler__SPError__) */
