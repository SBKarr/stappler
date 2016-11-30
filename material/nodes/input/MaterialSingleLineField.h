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

#ifndef MATERIAL_NODES_INPUT_MATERIALSINGLELINEFIELD_H_
#define MATERIAL_NODES_INPUT_MATERIALSINGLELINEFIELD_H_

#include "MaterialTextField.h"

NS_MD_BEGIN

class SingleLineField : public TextField {
public:
	using Callback = std::function<void()>;
	using Handler = ime::Handler;
	using Cursor = ime::Cursor;

	virtual bool init(FontType, float width = 0) override;
	virtual void onContentSizeDirty() override;

	virtual void setInputCallback(const Callback &);
	virtual const Callback &getInputCallback() const;

protected:
	virtual void onError(Error) override;

	virtual void updateFocus() override;
	virtual bool updateString(const std::u16string &str, const Cursor &c) override;

	Layer *_underlineLayer = nullptr;
	Callback _onInput;
};

NS_MD_END

#endif /* MATERIAL_NODES_INPUT_MATERIALSINGLELINEFIELD_H_ */
