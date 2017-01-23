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

#ifndef MATERIAL_NODES_FORM_MATERIALFORMCONTROLLER_H_
#define MATERIAL_NODES_FORM_MATERIALFORMCONTROLLER_H_

#include "Material.h"
#include "2d/CCComponent.h"

NS_MD_BEGIN

class FormController : public cocos2d::Component {
public:
	static EventHeader onForceCollect;
	static EventHeader onForceUpdate;

	using SaveCallback = Function<void(const data::Value &)>;

	virtual bool init(const data::Value & = data::Value(), const SaveCallback & = nullptr);
	virtual bool init(const Map<String, data::Value> &, const SaveCallback & = nullptr);

	virtual void onExit() override;

	virtual void setSaveCallback(const SaveCallback &);
	virtual const SaveCallback &getSaveCallback() const;

	virtual data::Value getValue(const String &) const;
	virtual void setValue(const String &, const data::Value &);

	virtual void reset();
	virtual void reset(const data::Value &);
	virtual void reset(const Map<String, data::Value> &);
	virtual data::Value collect(bool force = false);

	virtual void save(bool force = false);

protected:
	SaveCallback _saveCallback;
	Map<String, data::Value> _data;
};

NS_MD_END

#endif /* MATERIAL_NODES_FORM_MATERIALFORMCONTROLLER_H_ */
