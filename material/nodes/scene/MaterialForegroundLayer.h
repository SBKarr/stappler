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

#ifndef LIBS_MATERIAL_NODES_SCENE_MATERIALFOREGROUNDLAYER_H_
#define LIBS_MATERIAL_NODES_SCENE_MATERIALFOREGROUNDLAYER_H_

#include "Material.h"
#include "MaterialColors.h"
#include "2d/CCNode.h"
#include "base/CCVector.h"

NS_MD_BEGIN

class ForegroundLayer : public cocos2d::Node {
public:
	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual void pushNode(cocos2d::Node *, const Function<void()> & = nullptr);
	virtual void popNode(cocos2d::Node *);

	virtual void pushFloatNode(cocos2d::Node *, int);
	virtual void popFloatNode(cocos2d::Node *);

	virtual bool onPressBegin(const Vec2 &);
	virtual bool onPressEnd(const Vec2 &);

	virtual void clear();
	virtual bool isActive() const;

	virtual void setSnackbarString(const String &, const Color & = Color::White);
	virtual const String &getSnackbarString() const;

protected:
	virtual void setSnackbarStringInternal(const String &, const Color &color = Color::White);
	virtual void hideSnackbar(const Function<void()> & = nullptr);
	virtual void onSnackbarHidden();

	cocos2d::Node *_pressNode = nullptr;
	cocos2d::Component *_listener = nullptr;
	cocos2d::Vector<cocos2d::Node *> _nodes;
	Set<cocos2d::Node *> _pendingPush;
	Map<cocos2d::Node *, Function<void()>> _callbacks;

	Layer *_snackbar = nullptr;
	Label *_snackbarLabel = nullptr;
	String _snackbarString;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCENE_MATERIALFOREGROUNDLAYER_H_ */
