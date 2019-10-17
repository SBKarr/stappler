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
#include "MaterialFontSource.h"

NS_RT_BEGIN

Source::~Source() { }

Source::Source() {
	_scale = material::ResourceManager::getInstance()->getUserFontScale();
	onEvent(material::ResourceManager::onUserFont, std::bind(&Source::onFontSizeChanged, this));

	using namespace layout::style;

	addFontFaceMap(FontFaceMap{
		pair("default", Vector<FontFace>{
			FontFace(getSystemFont(material::SystemFontName::Roboto_Black), FontStyle::Normal, FontWeight::W800),
			FontFace(getSystemFont(material::SystemFontName::Roboto_BlackItalic), FontStyle::Italic, FontWeight::W800),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Bold), FontStyle::Normal, FontWeight::W700),
			FontFace(getSystemFont(material::SystemFontName::Roboto_BoldItalic), FontStyle::Italic, FontWeight::W700),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_Bold), FontStyle::Normal, FontWeight::W700, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_BoldItalic), FontStyle::Italic, FontWeight::W700, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_Italic), FontStyle::Italic, FontWeight::Normal, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_Light), FontStyle::Normal, FontWeight::W200, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_LightItalic), FontStyle::Italic, FontWeight::W200, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_Regular), FontStyle::Normal, FontWeight::Normal, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Italic), FontStyle::Italic),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Light), FontStyle::Normal, FontWeight::W200),
			FontFace(getSystemFont(material::SystemFontName::Roboto_LightItalic), FontStyle::Italic, FontWeight::W200),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Medium), FontStyle::Normal, FontWeight::W500),
			FontFace(getSystemFont(material::SystemFontName::Roboto_MediumItalic), FontStyle::Italic, FontWeight::W500),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Regular)),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Thin), FontStyle::Normal, FontWeight::W100),
			FontFace(getSystemFont(material::SystemFontName::Roboto_ThinItalic), FontStyle::Italic, FontWeight::W100)
		}),
		pair("system", Vector<FontFace>{
			FontFace(getSystemFont(material::SystemFontName::DejaVuSansStappler)),
		}),
		pair("monospace", Vector<FontFace>{
			FontFace(getSystemFont(material::SystemFontName::DejaVuSansMono)),
			FontFace(getSystemFont(material::SystemFontName::DejaVuSansMono_Bold), FontStyle::Normal, FontWeight::W700),
		}),
		pair("Roboto",  Vector<FontFace>{
			FontFace(getSystemFont(material::SystemFontName::Roboto_Black), FontStyle::Normal, FontWeight::W800),
			FontFace(getSystemFont(material::SystemFontName::Roboto_BlackItalic), FontStyle::Italic, FontWeight::W800),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Bold), FontStyle::Normal, FontWeight::W700),
			FontFace(getSystemFont(material::SystemFontName::Roboto_BoldItalic), FontStyle::Italic, FontWeight::W700),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_Bold), FontStyle::Normal, FontWeight::W700, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_BoldItalic), FontStyle::Italic, FontWeight::W700, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_Italic), FontStyle::Italic, FontWeight::Normal, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_Light), FontStyle::Normal, FontWeight::W200, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_LightItalic), FontStyle::Italic, FontWeight::W200, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::RobotoCondensed_Regular), FontStyle::Normal, FontWeight::Normal, FontStretch::Condensed),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Italic), FontStyle::Italic),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Light), FontStyle::Normal, FontWeight::W200),
			FontFace(getSystemFont(material::SystemFontName::Roboto_LightItalic), FontStyle::Italic, FontWeight::W200),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Medium), FontStyle::Normal, FontWeight::W500),
			FontFace(getSystemFont(material::SystemFontName::Roboto_MediumItalic), FontStyle::Italic, FontWeight::W500),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Regular)),
			FontFace(getSystemFont(material::SystemFontName::Roboto_Thin), FontStyle::Normal, FontWeight::W100),
			FontFace(getSystemFont(material::SystemFontName::Roboto_ThinItalic), FontStyle::Italic, FontWeight::W100)
		})
	});
}

void Source::onFontSizeChanged() {
	setFontScale(material::ResourceManager::getInstance()->getUserFontScale());
}

NS_RT_END
