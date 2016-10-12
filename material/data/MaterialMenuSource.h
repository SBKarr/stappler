/*
 * MaterialMenuSource.h
 *
 *  Created on: 07 янв. 2015 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_SRC_DATA_MATERIALMENUSOURCE_H_
#define MATERIAL_SRC_DATA_MATERIALMENUSOURCE_H_

#include "MaterialIconSources.h"
#include "SPData.h"
#include "SPDataSubscription.h"
#include "base/CCRef.h"
#include "base/CCVector.h"

#include <vector>
#include <functional>
#include <initializer_list>

NS_MD_BEGIN

class MenuSourceItem : public data::Subscription {
public:
	enum class Type {
		Separator,
		Button,
		Custom,
	};

	using AttachCallback = std::function<void(MenuSourceItem *, cocos2d::Node *)>;

	virtual bool init();
	virtual MenuSourceItem * copy();

	virtual void setCustomData(const stappler::data::Value &);
	virtual void setCustomData(stappler::data::Value &&);
	virtual const stappler::data::Value &getCustomData() const;

	virtual void setAttachCallback(const AttachCallback &);
	virtual void setDetachCallback(const AttachCallback &);

	virtual Type getType() const;

	virtual void onNodeAttached(cocos2d::Node *);
	virtual void onNodeDetached(cocos2d::Node *);

	void setDirty();

protected:
	Type _type = Type::Separator;
	stappler::data::Value _customData;

	AttachCallback _attachCallback;
	AttachCallback _detachCallback;
};

class MenuSourceCustom : public MenuSourceItem {
public:
	using FactoryFunction = std::function<cocos2d::Node *()>;

	virtual bool init();
	virtual bool init(float h, const FactoryFunction &func, bool relative = false);
	virtual MenuSourceItem * copy();

	virtual float getHeight() const;
	virtual const FactoryFunction & getFactoryFunction() const;

	virtual void setMinWidth(float w);
	virtual float getMinWidth() const;

	virtual void setRelativeHeight(bool value);
	virtual bool isRelativeHeight() const;
protected:
	float _height = 0.0f;
	float _minWidth = 0.0f;
	bool _relativeHeight = false;
	FactoryFunction _function = nullptr;
};

class MenuSource : public data::Subscription {
public:
	typedef std::function<void (Button *b, MenuSourceButton *)> Callback;

	virtual bool init() { return true; }
	virtual ~MenuSource();

	MenuSource *copy();

	void addItem(MenuSourceItem *);
	MenuSourceButton * addButton(const std::string &, const Callback & = nullptr);
	MenuSourceButton * addButton(const std::string &, IconName, const Callback & = nullptr);
	MenuSourceCustom * addCustom(float h, const MenuSourceCustom::FactoryFunction &func, bool rel = false);
	MenuSourceItem * addSeparator();

	void clear();

	uint32_t count();

	const cocos2d::Vector<MenuSourceItem *> &getItems();

protected:
	cocos2d::Vector<MenuSourceItem *> _items;
};

class MenuSourceButton : public MenuSourceItem {
public:
	using Callback = std::function<void (Button *b, MenuSourceButton *)>;

	virtual ~MenuSourceButton();

	virtual bool init(const std::string &, IconName, const Callback &);
	virtual bool init();
	virtual MenuSourceItem * copy();

	virtual void setName(const std::string &);
	virtual const std::string & getName() const;

	virtual void setValue(const std::string &);
	virtual const std::string & getValue() const;

	virtual void setNameIcon(IconName icon);
	virtual IconName getNameIcon() const;

	virtual void setValueIcon(IconName icon);
	virtual IconName getValueIcon() const;

	virtual void setCallback(const Callback &);
	virtual const Callback & getCallback() const;

	virtual void setNextMenu(MenuSource *);
	virtual MenuSource * getNextMenu() const;

	virtual void setSelected(bool value);
	virtual bool isSelected() const;
protected:
	std::string _name;
	std::string _value;

	IconName _nameIcon = IconName::None;
	IconName _valueIcon = IconName::None;

	Rc<MenuSource> _nextMenu = nullptr;
	Callback _callback = nullptr;
	bool _selected = false;
};

NS_MD_END

#endif /* MATERIAL_SRC_DATA_MATERIALMENUSOURCE_H_ */
