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

#ifndef MATERIAL_NODES_INPUT_MATERIALFORMFIELD_H_
#define MATERIAL_NODES_INPUT_MATERIALFORMFIELD_H_

#include "MaterialInputField.h"

NS_MD_BEGIN

class FormField : public InputField {
public:
	using SizeCallback = Function<void(const Size &)>;

	virtual bool init(bool dense = false);
	virtual bool init(FormController *, const String &name, bool dense = false);

	virtual void onContentSizeDirty() override;
	virtual void setContentSize(const Size &) override;

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual void setMaxChars(size_t) override;

	virtual void setSizeCallback(const SizeCallback &);
	virtual const SizeCallback &getSizeCallback() const;

	virtual void setCounterEnabled(bool);
	virtual bool isCounterEnabled() const;

	virtual void setAutoAdjust(bool);
	virtual bool isAutoAdjust() const;

	// should we show empty error/counter line?
	virtual void setFullHeight(bool);
	virtual bool isFullHeight() const;

	virtual void setError(const String &);
	virtual const String &getError() const;

protected:
	virtual void updateLabelHeight(float = nan());
	virtual Size getSizeForLabelWidth(float width, float labelHeight);

	virtual void onError(Error) override;
	virtual void onCursor(const Cursor &c) override;
	virtual void onInput() override;
	virtual void onActivated(bool) override;
	virtual void updateAutoAdjust();

	virtual void pushFormData();
	virtual void updateFormData();

	bool _autoAdjust = true;
	bool _fullHeight = false;
	ScrollView *_adjustScroll = nullptr;

	bool _dense = false;
	bool _counterEnabled = false;
	float _labelHeight = 0.0f;
	Label *_counter = nullptr;
	Label *_error = nullptr;
	Layer *_underlineLayer = nullptr;
	SizeCallback _sizeCallback;
	Padding _padding = Padding(12.0f, 16.0f);

	FormController *_formController = nullptr;
	String _formName;
};

NS_MD_END

#endif /* MATERIAL_NODES_INPUT_MATERIALFORMFIELD_H_ */
