// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "Material.h"
#include "MaterialResourceManager.h"
#include "RTSource.h"

NS_RT_BEGIN

Source::~Source() { }

Source::Source() {
	_scale = material::ResourceManager::getInstance()->getUserFontScale();
	onEvent(material::ResourceManager::onUserFont, std::bind(&Source::onFontSizeChanged, this));

	using namespace layout::style;

	addFontFaceMap(FontFaceMap{
		pair("default",  Vector<FontFace>{
			FontFace("Roboto-Black.woff", FontStyle::Normal, FontWeight::W800),
			FontFace("Roboto-BlackItalic.woff", FontStyle::Italic, FontWeight::W800),
			FontFace("Roboto-Bold.woff", FontStyle::Normal, FontWeight::W700),
			FontFace("Roboto-BoldItalic.woff", FontStyle::Italic, FontWeight::W700),
			FontFace("RobotoCondensed-Bold.woff", FontStyle::Normal, FontWeight::W700, FontStretch::Condensed),
			FontFace("RobotoCondensed-BoldItalic.woff", FontStyle::Italic, FontWeight::W700, FontStretch::Condensed),
			FontFace("RobotoCondensed-Italic.woff", FontStyle::Italic, FontWeight::Normal, FontStretch::Condensed),
			FontFace("RobotoCondensed-Light.woff", FontStyle::Normal, FontWeight::W200, FontStretch::Condensed),
			FontFace("RobotoCondensed-LightItalic.woff", FontStyle::Italic, FontWeight::W200, FontStretch::Condensed),
			FontFace("RobotoCondensed-Regular.woff", FontStyle::Normal, FontWeight::Normal, FontStretch::Condensed),
			FontFace("Roboto-Italic.woff", FontStyle::Italic),
			FontFace("Roboto-Light.woff", FontStyle::Normal, FontWeight::W200),
			FontFace("Roboto-LightItalic.woff", FontStyle::Italic, FontWeight::W200),
			FontFace("Roboto-Medium.woff", FontStyle::Normal, FontWeight::W500),
			FontFace("Roboto-MediumItalic.woff", FontStyle::Italic, FontWeight::W500),
			FontFace("Roboto-Regular.woff"),
			FontFace("Roboto-Thin.woff", FontStyle::Normal, FontWeight::W100),
			FontFace("Roboto-ThinItalic.woff", FontStyle::Italic, FontWeight::W100),
		}),
		pair("system", Vector<FontFace>{
			FontFace("DejaVuSansStappler.woff"),
		}),
		pair("monospace", Vector<FontFace>{
			FontFace("DejaVuSansMono.woff"),
			FontFace("DejaVuSansMono-Bold.woff", FontStyle::Normal, FontWeight::W700),
		}),
		pair("Roboto",  Vector<FontFace>{
			FontFace("Roboto-Black.woff", FontStyle::Normal, FontWeight::W800),
			FontFace("Roboto-BlackItalic.woff", FontStyle::Italic, FontWeight::W800),
			FontFace("Roboto-Bold.woff", FontStyle::Normal, FontWeight::W700),
			FontFace("Roboto-BoldItalic.woff", FontStyle::Italic, FontWeight::W700),
			FontFace("RobotoCondensed-Bold.woff", FontStyle::Normal, FontWeight::W700, FontStretch::Condensed),
			FontFace("RobotoCondensed-BoldItalic.woff", FontStyle::Italic, FontWeight::W700, FontStretch::Condensed),
			FontFace("RobotoCondensed-Italic.woff", FontStyle::Italic, FontWeight::Normal, FontStretch::Condensed),
			FontFace("RobotoCondensed-Light.woff", FontStyle::Normal, FontWeight::W200, FontStretch::Condensed),
			FontFace("RobotoCondensed-LightItalic.woff", FontStyle::Italic, FontWeight::W200, FontStretch::Condensed),
			FontFace("RobotoCondensed-Regular.woff", FontStyle::Normal, FontWeight::Normal, FontStretch::Condensed),
			FontFace("Roboto-Italic.woff", FontStyle::Italic),
			FontFace("Roboto-Light.woff", FontStyle::Normal, FontWeight::W200),
			FontFace("Roboto-LightItalic.woff", FontStyle::Italic, FontWeight::W200),
			FontFace("Roboto-Medium.woff", FontStyle::Normal, FontWeight::W500),
			FontFace("Roboto-MediumItalic.woff", FontStyle::Italic, FontWeight::W500),
			FontFace("Roboto-Regular.woff"),
			FontFace("Roboto-Thin.woff", FontStyle::Normal, FontWeight::W100),
			FontFace("Roboto-ThinItalic.woff", FontStyle::Italic, FontWeight::W100),
		})
	});
}

void Source::onFontSizeChanged() {
	setFontScale(material::ResourceManager::getInstance()->getUserFontScale());
}

NS_RT_END
