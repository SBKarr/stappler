/*
 * MaterialContentLayer.h
 *
 *  Created on: 16 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_SCENE_MATERIALCONTENTLAYER_H_
#define LIBS_MATERIAL_NODES_SCENE_MATERIALCONTENTLAYER_H_

#include "MaterialLayout.h"
#include "2d/CCLayer.h"
#include "2d/CCActionInterval.h"
#include "base/CCMap.h"

NS_MD_BEGIN

class ContentLayer : public cocos2d::Layer {
public:
	using Transition = cocos2d::FiniteTimeAction;

	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual void replaceNode(Layout *, Transition *enterTransition = nullptr);
	virtual void pushNode(Layout *, Transition *enterTransition = nullptr, Transition *exitTransition = nullptr);
	virtual void replaceTopNode(Layout *, Transition *enterTransition = nullptr, Transition *exitTransition = nullptr);
	virtual void popNode(Layout *);

	virtual Layout *getRunningNode();
	virtual Layout *getPrevNode();

	virtual bool popLastNode();
	virtual bool isActive() const;

	virtual bool onBackButton();
	virtual size_t getNodesCount() const;

protected:
	virtual void eraseNode(Layout *);
	virtual void replaceNodes();
	virtual void updateNodesVisibility();

	cocos2d::Component *_listener = nullptr;
	cocos2d::Vector<Layout *> _nodes;
	cocos2d::Map<Layout *, Transition *> _exitTransitions;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCENE_MATERIALCONTENTLAYER_H_ */
