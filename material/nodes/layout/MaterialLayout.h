/*
 * MaterialLayout.h
 *
 *  Created on: 25 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_LAYOUT_MATERIALLAYOUT_H_
#define LIBS_MATERIAL_NODES_LAYOUT_MATERIALLAYOUT_H_

#include "Material.h"
#include "SPStrictNode.h"

NS_MD_BEGIN

class Layout : public stappler::StrictNode {
public:
	using BackButtonCallback = std::function<bool()>;

	virtual ~Layout() { }
	virtual bool onBackButton();

	virtual void setBackButtonCallback(const BackButtonCallback &);
	virtual const BackButtonCallback &getBackButtonCallback() const;

	virtual void onPush(ContentLayer *l, bool replace);
	virtual void onPushTransitionEnded(ContentLayer *l, bool replace);

	virtual void onPopTransitionBegan(ContentLayer *l, bool replace);
	virtual void onPop(ContentLayer *l, bool replace);

	virtual void onBackground(ContentLayer *l, Layout *overlay);
	virtual void onBackgroundTransitionEnded(ContentLayer *l, Layout *overlay);

	virtual void onForegroundTransitionBegan(ContentLayer *l, Layout *overlay);
	virtual void onForeground(ContentLayer *l, Layout *overlay);

protected:
	BackButtonCallback _backButtonCallback;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_LAYOUT_MATERIALLAYOUT_H_ */
