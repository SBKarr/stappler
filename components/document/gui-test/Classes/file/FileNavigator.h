/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef CLASSES_APPLICATION_FILENAVIGATOR_H_
#define CLASSES_APPLICATION_FILENAVIGATOR_H_

#include "MaterialToolbarLayout.h"
#include "SPDataSource.h"
#include "SPFont.h"

NS_SP_EXT_BEGIN(app)

class FileNavigator : public material::ToolbarLayout {
public:
	virtual bool init() override;

	virtual void refreshData();
	virtual void updateData(const StringView &path);

	virtual void onForeground(material::ContentLayer *l, Layout *overlay) override;

protected:
	virtual void onButton(const StringView &);
	virtual void onDirButton(const StringView &);
	virtual void openFile(const StringView &);
	virtual void openDirectory();
	virtual void switchHidden();

	material::Scroll *_scroll;
	material::MenuSourceButton *_switchHiddenButton;

	bool _showHidden = false;
	std::string _path;
	data::Value _data;
	Rc<data::Source> _source;
	Rc<font::HyphenMap> _hyphens;
};

NS_SP_EXT_END(app)

#endif /* CLASSES_APPLICATION_FILENAVIGATOR_H_ */
