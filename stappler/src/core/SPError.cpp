//
//  SPError.cpp
//  stappler
//
//  Created by SBKarr on 8/7/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

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
	bool isTouched(cocos2d::Node *node, const cocos2d::Vec2 &location, float padding) {
#ifndef SP_RESTRICT
		const cocos2d::Vec2 &point = node->convertToNodeSpace(location);
		if (point.x > -padding && point.y > -padding
				&& point.x < node->getContentSize().width + padding
				&& point.y < node->getContentSize().height + padding) {
			return true;
		} else {
			return false;
		}
#else
		return false;
#endif
	}

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
