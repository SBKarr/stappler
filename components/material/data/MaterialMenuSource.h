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

#ifndef MATERIAL_SRC_DATA_MATERIALMENUSOURCE_H_
#define MATERIAL_SRC_DATA_MATERIALMENUSOURCE_H_

#include "MaterialIconStorage.h"
#include "SPData.h"

NS_MD_BEGIN

class MenuSourceItem : public data::Subscription {
public:
	enum class Type {
		Separator,
		Button,
		Custom,
	};

	using AttachCallback = Function<void(MenuSourceItem *, cocos2d::Node *)>;

	virtual bool init();
	virtual Rc<MenuSourceItem> copy() const;

	virtual void setCustomData(const data::Value &);
	virtual void setCustomData(data::Value &&);
	virtual const data::Value &getCustomData() const;

	virtual MenuSourceItem *setAttachCallback(const AttachCallback &);
	virtual MenuSourceItem *setDetachCallback(const AttachCallback &);

	virtual Type getType() const;

	virtual void onNodeAttached(cocos2d::Node *);
	virtual void onNodeDetached(cocos2d::Node *);

	void setDirty();

protected:
	Type _type = Type::Separator;
	data::Value _customData;

	AttachCallback _attachCallback;
	AttachCallback _detachCallback;
};

class MenuSourceCustom : public MenuSourceItem {
public:
	using FactoryFunction = Function<Rc<cocos2d::Node>()>;
	using HeightFunction = Function<float(float)>;

	virtual bool init() override;
	virtual bool init(float h, const FactoryFunction &func, float minWidth = 0.0f);
	virtual bool init(const HeightFunction &h, const FactoryFunction &func, float minWidth = 0.0f);

	virtual Rc<MenuSourceItem> copy() const override;

	virtual float getMinWidth() const;
	virtual float getHeight(float) const;
	virtual const HeightFunction & getHeightFunction() const;
	virtual const FactoryFunction & getFactoryFunction() const;

protected:
	float _minWidth = 0.0f;
	HeightFunction _heightFunction = nullptr;
	FactoryFunction _function = nullptr;
};

class MenuSource : public data::Subscription {
public:
	using Callback = Function<void (Button *b, MenuSourceButton *)>;
	using FactoryFunction = MenuSourceCustom::FactoryFunction;
	using HeightFunction = MenuSourceCustom::HeightFunction;

	virtual bool init() { return true; }
	virtual ~MenuSource();

	void setHintCount(size_t);
	size_t getHintCount() const;

	Rc<MenuSource> copy() const;

	void addItem(MenuSourceItem *);
	Rc<MenuSourceButton> addButton(const String &, const Callback & = nullptr);
	Rc<MenuSourceButton> addButton(const String &, IconName, const Callback & = nullptr);
	Rc<MenuSourceCustom> addCustom(float h, const FactoryFunction &func, float minWidth = 0.0f);
	Rc<MenuSourceCustom> addCustom(const HeightFunction &h, const FactoryFunction &func, float minWidth = 0.0f);
	Rc<MenuSourceItem> addSeparator();

	void clear();

	uint32_t count();

	const Vector<Rc<MenuSourceItem>> &getItems() const;

protected:
	Vector<Rc<MenuSourceItem>> _items;
	size_t _hintCount = 0;
};

class MenuSourceButton : public MenuSourceItem {
public:
	using Callback = std::function<void (Button *b, MenuSourceButton *)>;

	virtual ~MenuSourceButton();

	virtual bool init(const String &, IconName, const Callback &);
	virtual bool init() override;
	virtual Rc<MenuSourceItem> copy() const override;

	virtual void setName(const String &);
	virtual const String & getName() const;

	virtual void setValue(const String &);
	virtual const String & getValue() const;

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
	String _name;
	String _value;

	IconName _nameIcon = IconName::None;
	IconName _valueIcon = IconName::None;

	Rc<MenuSource> _nextMenu = nullptr;
	Callback _callback = nullptr;
	bool _selected = false;
};

NS_MD_END

#endif /* MATERIAL_SRC_DATA_MATERIALMENUSOURCE_H_ */
