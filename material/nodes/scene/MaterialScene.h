/*
 * MaterialScene.h
 *
 *  Created on: 05 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_SCENE_MATERIALSCENE_H_
#define LIBS_MATERIAL_NODES_SCENE_MATERIALSCENE_H_

#include "Material.h"
#include "MaterialColors.h"
#include "MaterialContentLayer.h"
#include "MaterialBackgroundLayer.h"
#include "MaterialNavigationLayer.h"
#include "MaterialForegroundLayer.h"
#include "SPDynamicBatchScene.h"
#include <set>

/* Default scene layout:
 *
 * --- Foreground
 * 			All popups and floating menus must be placed here
 * 			to capture touch input on the top of scene graph
 *
 * --- Navigation
 * 			Special level for navigation drawer
 *
 * --- Content
 * 			All content pages should be on this level
 *
 * --- Background
 * 			Global background, usually for color or image background
 *
 */

NS_MD_BEGIN

class Scene : public DynamicBatchScene {
public:
	static stappler::EventHeader onBackKey;

public:
	static material::Scene *getRunningScene();
	static void run(Scene *scene = nullptr);

	virtual bool init() override;
	virtual void onEnter() override;
	virtual void onEnterTransitionDidFinish() override;
	virtual void onExitTransitionDidStart() override;

	virtual const cocos2d::Size &getViewSize() const;

	virtual ForegroundLayer *getForegroundLayer() const;
	virtual NavigationLayer *getNavigationLayer() const;
	virtual ContentLayer *getContentLayer() const;
	virtual BackgroundLayer *getBackgroundLayer() const;

	virtual void setAutoLightLevel(bool value);
	virtual bool isAutoLightLevel() const;

	virtual void setDisplayStats(bool value);
	virtual bool isDisplayStats() const;

	virtual void setVisualizeTouches(bool value);
	virtual bool isVisualizeTouches() const;

	virtual void update(float dt) override;

	virtual void captureContentForNode(cocos2d::Node *);
	virtual void releaseContentForNode(cocos2d::Node *);
	virtual bool isContentCaptured();

	virtual void replaceContentNode(Layout *, cocos2d::FiniteTimeAction *enterTransition = nullptr);
	virtual void replaceTopContentNode(Layout *, cocos2d::FiniteTimeAction *enterTransition = nullptr, cocos2d::FiniteTimeAction *exitTransition = nullptr);
	virtual void pushContentNode(Layout *, cocos2d::FiniteTimeAction *enterTransition = nullptr, cocos2d::FiniteTimeAction *exitTransition = nullptr);
	virtual void popContentNode(Layout *);

	virtual void pushForegroundNode(cocos2d::Node *, const std::function<void()> & = nullptr);
	virtual void popForegroundNode(cocos2d::Node *);

	virtual void setNavigationMenuSource(material::MenuSource *);
	virtual void setSnackbarString(const std::string &, const Color & = Color::White);

protected:
	virtual ForegroundLayer *createForegroundLayer();
	virtual NavigationLayer *createNavigationLayer();
	virtual ContentLayer *createContentLayer();
	virtual BackgroundLayer *createBackgroundLayer();

	virtual void layoutSubviews();
	virtual void layoutLayer(cocos2d::Node *);

	virtual void onLightLevel();

	void updateStatsLabel();
	void updateStatsValue();
	void updateCapturedContent();

	void onBackKeyPressed();
	void takeScreenshoot();

	ForegroundLayer *_foreground = nullptr;
	NavigationLayer *_navigation = nullptr;
	ContentLayer *_content = nullptr;
	BackgroundLayer *_background = nullptr;

	Label *_stats = nullptr;
	cocos2d::LayerColor *_statsColor = nullptr;

	bool _autoLightLevel = false;
	bool _visualizeTouches = false;
	bool _displayStats = false;
	bool _registred = false;

	bool _contentCaptured = false;
	std::set<cocos2d::Node *> _contentCaptureNodes;

	cocos2d::Node *_touchesNode = nullptr;
	std::map<int, cocos2d::Node *> _touchesNodes;

	bool _contentCapturing = false;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCENE_MATERIALSCENE_H_ */
