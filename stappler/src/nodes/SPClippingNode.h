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

#ifndef LIBS_STAPPLER_NODES_SPCLIPPINGNODE_H_
#define LIBS_STAPPLER_NODES_SPCLIPPINGNODE_H_

#include "SPDefine.h"
#include "2d/CCClippingNode.h"

NS_SP_BEGIN

class ClippingNode : public cocos2d::ClippingNode {
public:
    virtual bool init(Node *stencil = nullptr) override;

    virtual void setEnabled(bool value);
    virtual bool isEnabled() const;

    virtual void visit(cocos2d::Renderer *, const cocos2d::Mat4 &, uint32_t, ZPath &zPath) override;

protected:
	bool _enabled = true;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_SPCLIPPINGNODE_H_ */
