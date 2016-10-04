/*
 * MaterialSearchToolbar.h
 *
 *  Created on: 6 окт. 2016 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_NODES_TOOLBAR_MATERIALSEARCHTOOLBAR_H_
#define MATERIAL_NODES_TOOLBAR_MATERIALSEARCHTOOLBAR_H_

#include "MaterialToolbar.h"

NS_MD_BEGIN

class SearchToolbar : public Toolbar {
public:
	virtual ~SearchToolbar() { }

	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual void setPlaceholder(const String &);

	virtual void onSearchMenuButton();
	virtual void onSearchIconButton();

protected:
	virtual void layoutSubviews() override;

	ButtonIcon *_searchIcon = nullptr;
	SingleLineField *_searchInput = nullptr;
	bool _presistentSearch = false;
	bool _inSearchMode = false;
};

NS_MD_END

#endif /* MATERIAL_NODES_TOOLBAR_MATERIALSEARCHTOOLBAR_H_ */
