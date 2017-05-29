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

#ifndef STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPSTENCILCACHE_H_
#define STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPSTENCILCACHE_H_

#include "SPDefine.h"

NS_SP_BEGIN

class StencilCache {
public:
	static StencilCache *getInstance();

	enum class Func {
		Never,
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Equal,
		NotEqual,
		Always,
	};

	enum class Op {
		Keep,
		Zero,
		Replace,
		Incr,
		IncrWrap,
		Decr,
		DecrWrap,
		Invert,
	};

	struct State {
		bool enabled = false;

		uint8_t writeMask = 0;

		Func func = Func::Always;
		uint8_t ref = 0;
		uint8_t valueMask = 0;

		Op stencilFailOp = Op::Keep;
		Op depthFailOp = Op::Keep;
		Op passOp = Op::Keep;
	};

	bool isEnabled() const;

	void enable(uint8_t mask = maxOf<uint8_t>());
	void disable();

	void clear();

	uint8_t pushStencilLayer();

	void enableStencilDrawing(uint8_t value, uint8_t mask = maxOf<uint8_t>());
	void enableStencilTest(Func, uint8_t value, uint8_t mask = maxOf<uint8_t>());

	void disableStencilTest(uint8_t mask = maxOf<uint8_t>());

protected:
	State saveState() const;
	void restoreState(State st) const;

	void drawFullScreenQuadClearStencil(uint8_t mask, bool inverted);

	void setFunc(Func, uint8_t value, uint8_t mask);
	void setOp(Op, Op, Op);

	bool _enabled = false;
	bool _inverted = false;

	uint8_t _value = 0;
	State _savedState;
	State _currentState;
};

NS_SP_END

#endif /* STAPPLER_SRC_FEATURES_DYNAMIC_ATLAS_SPSTENCILCACHE_H_ */
