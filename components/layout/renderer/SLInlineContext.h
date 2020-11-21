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

#ifndef LAYOUT_RENDERER_SLINLINECONTEXT_H_
#define LAYOUT_RENDERER_SLINLINECONTEXT_H_

#include "SLResultObject.h"

NS_LAYOUT_BEGIN

struct InlineContext : public Ref {
	using NodeCallback = Function<void(InlineContext &ctx)>;

	struct RefPosInfo {
		uint16_t firstCharId;
		uint16_t lastCharId;

		String target;
		String mode;
	};

	struct OutlinePosInfo {
		uint16_t firstCharId;
		uint16_t lastCharId;

		OutlineStyle style;
	};

	struct BackgroundPosInfo {
		uint16_t firstCharId;
		uint16_t lastCharId;

		BackgroundStyle background;
		Padding padding;
	};

	struct IdPosInfo {
		uint16_t firstCharId;
		uint16_t lastCharId;

		StringView id;
	};

	static void initFormatter(Layout &l, const FontParameters &fStyle, const ParagraphStyle &pStyle, float parentPosY, Formatter &reader);
	static void initFormatter(Layout &l, float parentPosY, Formatter &reader);

	InlineContext();

	bool init(FontSource *, float);

	void setTargetLabel(Label *);

	void pushNode(const Node *, const NodeCallback &);
	void popNode(const Node *);
	void finalize(Layout &);
	void finalize();
	void reset();

	Layout * alignInlineContext(Layout &inl, const Vec2 &);

	Vector<RefPosInfo> refPos;
	Vector<OutlinePosInfo> outlinePos;
	Vector<BackgroundPosInfo> backgroundPos;
	Vector<IdPosInfo> idPos;

	float density = 1.0f;
	float lineHeightMod = 1.0f;
	bool lineHeightIsAbsolute = false;
	uint16_t lineHeight = 0;

	Label phantomLabel;
	Label *targetLabel = nullptr;
	Formatter reader;
	bool finalized = false;

	Vector<Pair<const Node *, NodeCallback>> nodes;

};

NS_LAYOUT_END

#endif /* LAYOUT_RENDERER_SLINLINECONTEXT_H_ */
