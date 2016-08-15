/*
 * MaterialMenu.h
 *
 *  Created on: 07 янв. 2015 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_SRC_NODES_MATERIALMENU_H_
#define MATERIAL_SRC_NODES_MATERIALMENU_H_

#include "MaterialButton.h"
#include "MaterialMenuSource.h"

NS_MD_BEGIN

class Menu : public MaterialNode {
public:
	using ButtonCallback = std::function<void(MenuButton *button)>;

	virtual ~Menu();

	virtual bool init() override;
	virtual bool init(MenuSource *source);

	virtual void setMenuSource(MenuSource *);
	virtual MenuSource *getMenuSource() const;

	virtual void setEnabled(bool value);
	virtual bool isEnabled() const;

	virtual void setMetrics(MenuMetrics spacing);
	virtual MenuMetrics getMetrics();

	virtual void setMenuButtonCallback(const ButtonCallback &cb);
	virtual const ButtonCallback & getMenuButtonCallback();

	virtual void onMenuButtonPressed(MenuButton *button);
	virtual void layoutSubviews();

	virtual void onContentSizeDirty() override;

	virtual stappler::ScrollView *getScroll() const;
protected:
	virtual void rebuildMenu();
	virtual MenuButton *createButton();
	virtual MenuSeparator *createSeparator();

	virtual void onLightLevel() override;
	virtual void onSourceDirty();

	ScrollView *_scroll = nullptr;
	ScrollController *_controller = nullptr;
	data::Listener<MenuSource> _menu;
	MenuMetrics _metrics;

	ButtonCallback _callback = nullptr;
};

NS_MD_END

#endif /* MATERIAL_SRC_NODES_MATERIALMENU_H_ */
