/*
 * MaterialSingleLineInput.h
 *
 *  Created on: 13 окт. 2015 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_NODES_INPUT_MATERIALSINGLELINEFIELD_H_
#define MATERIAL_NODES_INPUT_MATERIALSINGLELINEFIELD_H_

#include "MaterialTextField.h"

NS_MD_BEGIN

class SingleLineField : public TextField {
public:
	using Callback = std::function<void()>;
	using Handler = ime::Handler;
	using Cursor = ime::Cursor;

	virtual bool init(const Font *, float width = 0) override;
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
