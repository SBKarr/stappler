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

#ifndef RICH_TEXT_COMMON_RTCOMMON_H_
#define RICH_TEXT_COMMON_RTCOMMON_H_

#include "SPFont.h"
#include "SLRendererTypes.h"
#include "SLResult.h"
#include "SLDocument.h"

#define NS_RT_BEGIN NS_SP_EXT_BEGIN(rich_text)
#define NS_RT_END NS_SP_EXT_END(rich_text)

NS_RT_BEGIN

using FontSource = font::FontSource;
using MediaParameters = layout::MediaParameters;
using MediaResolver = layout::MediaResolver;
using Result = layout::Result;
using PageData = layout::Result::PageData;
using Object = layout::Object;
using Layout = layout::Layout;
using Document = layout::Document;

using Background = layout::BackgroundStyle;
using Label = layout::Label;
using Link = layout::Link;

class CommonSource;
class Source;
class CommonView;
class ListenerView;
class View;
class Tooltip;
class Source;
class ImageView;

class EpubContentsView;
class EpubNavigation;
class EpubView;

NS_RT_END

#endif /* RICH_TEXT_RTCOMMON_H_ */
