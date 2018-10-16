/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef CLASSES_APPLICATION_APPLICATION_H_
#define CLASSES_APPLICATION_APPLICATION_H_

#include "SPScheme.h"
#include "SPEventHandler.h"
#include "SPData.h"
#include "Material.h"
#include "SPIME.h"
#include "MaterialMenuSource.h"

NS_SP_EXT_BEGIN(app)

class Application : stappler::EventHandler {
public:
	static Application *getInstance();

	Application();
	~Application();

	void init();

	void setData(const data::Value &);

protected:
	void rebuildMenu();

	void saveData();

	bool _enabled = true;

	data::Value _data;
	Rc<material::MenuSource> _appMenu;
};

NS_SP_EXT_END(app)

#endif /* CLASSES_APPLICATION_APPLICATION_H_ */
