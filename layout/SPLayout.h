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

#ifndef LAYOUT_SPLAYOUT_H_
#define LAYOUT_SPLAYOUT_H_

/* Stappler Layout Engine Sources
 *
 * Layout engine for rich text applications
 */

#include "SPCommon.h"
#include "SPRef.h"

#ifdef __cplusplus

#define NS_LAYOUT_BEGIN		namespace stappler { namespace layout {
#define NS_LAYOUT_END		} }
#define USING_NS_LAYOUT		using namespace stappler::layout

#else

#define NS_LAYOUT_BEGIN
#define NS_LAYOUT_END
#define USING_NS_LAYOUT

#endif

#define LAYOUT_COLOR_SPEC_BASE(Name) \
	static Color Name ## _50; \
	static Color Name ## _100; \
	static Color Name ## _200; \
	static Color Name ## _300; \
	static Color Name ## _400; \
	static Color Name ## _500; \
	static Color Name ## _600; \
	static Color Name ## _700; \
	static Color Name ## _800; \
	static Color Name ## _900;

#define LAYOUT_COLOR_SPEC_ACCENT(Name) \
	static Color Name ## _A100; \
	static Color Name ## _A200; \
	static Color Name ## _A400; \
	static Color Name ## _A700;

#define LAYOUT_COLOR_SPEC(Name) \
		LAYOUT_COLOR_SPEC_BASE(Name) \
		LAYOUT_COLOR_SPEC_ACCENT(Name)

NS_LAYOUT_BEGIN

constexpr uint32_t EngineVersion() { return 3; }

using Ref = stappler::Ref;

class Formatter;
class Document;
class Result;
class Node;
class Reader;

struct MultipartParser;
struct MediaParameters;
class MediaResolver;

struct Metrics;
struct CharLayout;
struct CharSpec;
struct FontCharString;
struct FontData;
struct CharTexture;

class FontLayout;
class FontSource;
class FontController;
class HyphenMap;

class Path;
class Canvas;
class Image;

NS_LAYOUT_END

#include "SPLog.h"
#include "SPTime.h"
#include "SPData.h"
#include "SPString.h"

#include "SLMat4.h"
#include "SLQuaternion.h"
#include "SLVec2.h"
#include "SLVec3.h"
#include "SLVec4.h"
#include "SLColor.h"
#include "SLRef.h"

#include "SLPadding.h"
#include "SLMovingAverage.h"
#include "SLStyle.h"
#include "SLSubscription.h"

#endif /* LAYOUT_SPLAYOUT_H_ */
