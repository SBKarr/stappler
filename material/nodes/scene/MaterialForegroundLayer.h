/*
 * ForegroundLayer.h
 *
 *  Created on: 16 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_SCENE_MATERIALFOREGROUNDLAYER_H_
#define LIBS_MATERIAL_NODES_SCENE_MATERIALFOREGROUNDLAYER_H_

#include "Material.h"
#include "MaterialColors.h"
#include "2d/CCLayer.h"
#include "base/CCVector.h"

NS_MD_BEGIN

class ForegroundLayer : public cocos2d::Layer {
public:
	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual void pushNode(cocos2d::Node *, const std::function<void()> & = nullptr);
	virtual void popNode(cocos2d::Node *);

	virtual bool onPressBegin(const cocos2d::Vec2 &);
	virtual bool onPressEnd(const cocos2d::Vec2 &);

	virtual void clear();
	virtual bool isActive() const;

	virtual void setSnackbarString(const std::string &, const Color & = Color::White);
	virtual const std::string &getSnackbarString() const;

protected:
	virtual void setSnackbarStringInternal(const std::string &, const Color &color = Color::White);
	virtual void hideSnackbar(const std::function<void()> & = nullptr);
	virtual void onSnackbarHidden();

	cocos2d::Node *_pressNode = nullptr;
	cocos2d::Component *_listener = nullptr;
	cocos2d::Vector<cocos2d::Node *> _nodes;
	std::set<cocos2d::Node *> _pendingPush;
	std::map<cocos2d::Node *, std::function<void()>> _callbacks;

	stappler::Layer *_snackbar = nullptr;
	Label *_snackbarLabel = nullptr;
	std::string _snackbarString;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCENE_MATERIALFOREGROUNDLAYER_H_ */
