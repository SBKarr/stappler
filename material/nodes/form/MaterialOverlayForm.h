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

#ifndef MATERIAL_NODES_FORM_MATERIALOVERLAYFORM_H_
#define MATERIAL_NODES_FORM_MATERIALOVERLAYFORM_H_

#include "MaterialNode.h"
#include "MaterialFormController.h"

NS_MD_BEGIN

class OverlayForm : public MaterialNode {
public:
	// If callback returns true, action was performed and we should close form
	// if callback returns false, we shold not close form
	using CompleteCallback = Function<bool(bool, data::Value &&)>;

	// If callback returns true - submit was successful, we wait for complete call
	// If false - show errors, if any
	using SubmitCallback = Function<bool(data::Value &&, const CompleteCallback &)>;

	// save callback from form controller
	using SaveCallback = Function<void(const data::Value &)>;

	virtual bool init(const data::Value &, float width);
	virtual void onContentSizeDirty() override;

	virtual void setCompleteCallback(const CompleteCallback &);
	virtual const CompleteCallback &getCompleteCallback() const;

	virtual void setSubmitCallback(const SubmitCallback &);
	virtual const SubmitCallback &getSubmitCallback() const;

	virtual void setSaveCallback(const SaveCallback &);
	virtual const SaveCallback &getSaveCallback() const;

	virtual bool isOpen() const;

	virtual void push();

	// point in global coords (like in Button::getTouchPoint)
	virtual void push(const Vec2 &);

	virtual void close();
	virtual bool submit();

protected:
	virtual bool complete(bool, data::Value &&);
	virtual void onOpen();

	Rc<FormController> _formController;
	OverlayLayout *_layout = nullptr;
	ScrollView *_scroll = nullptr;

	bool _isOpen = false;
	float _basicWidth = 0.0f;

	CompleteCallback _completeCallback;
	SubmitCallback _submitCallback;
};

NS_MD_END

#endif /* MATERIAL_NODES_FORM_MATERIALOVERLAYFORM_H_ */
