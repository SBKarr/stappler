/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

// Generated with FontGenerator (%STAPPLER_ROOT%/test/generators/FontGenerator)

#include "Material.h"
#include "MaterialFontSource.h"

#include "MaterialFont-DejaVuSansMono.cc"
#include "MaterialFont-DejaVuSansMono-Bold.cc"
//#include "MaterialFont-DejaVuSansStappler.cc"
#include "MaterialFont-Roboto-Black.cc"
#include "MaterialFont-Roboto-BlackItalic.cc"
#include "MaterialFont-Roboto-Bold.cc"
#include "MaterialFont-Roboto-BoldItalic.cc"
#include "MaterialFont-Roboto-Italic.cc"
#include "MaterialFont-Roboto-Light.cc"
#include "MaterialFont-Roboto-LightItalic.cc"
#include "MaterialFont-Roboto-Medium.cc"
#include "MaterialFont-Roboto-MediumItalic.cc"
#include "MaterialFont-Roboto-Regular.cc"
#include "MaterialFont-Roboto-Thin.cc"
#include "MaterialFont-Roboto-ThinItalic.cc"
#include "MaterialFont-RobotoCondensed-Bold.cc"
#include "MaterialFont-RobotoCondensed-BoldItalic.cc"
#include "MaterialFont-RobotoCondensed-Italic.cc"
#include "MaterialFont-RobotoCondensed-Light.cc"
#include "MaterialFont-RobotoCondensed-LightItalic.cc"
#include "MaterialFont-RobotoCondensed-Regular.cc"

NS_SP_EXT_BEGIN(resource)
extern BytesView extern_font_DejaVuSansStappler;
NS_SP_EXT_END(resource)

NS_MD_BEGIN

BytesView getSystemFont(SystemFontName name) {
	switch (name) {
		case SystemFontName::DejaVuSansMono: return BytesView(s_font_DejaVuSansMono, 196176); break;
		case SystemFontName::DejaVuSansMono_Bold: return BytesView(s_font_DejaVuSansMono_Bold, 191148); break;
		case SystemFontName::DejaVuSansStappler: return resource::extern_font_DejaVuSansStappler; break;
		case SystemFontName::Roboto_Black: return BytesView(s_font_Roboto_Black, 66448); break;
		case SystemFontName::Roboto_BlackItalic: return BytesView(s_font_Roboto_BlackItalic, 73468); break;
		case SystemFontName::Roboto_Bold: return BytesView(s_font_Roboto_Bold, 66588); break;
		case SystemFontName::Roboto_BoldItalic: return BytesView(s_font_Roboto_BoldItalic, 73340); break;
		case SystemFontName::Roboto_Italic: return BytesView(s_font_Roboto_Italic, 72348); break;
		case SystemFontName::Roboto_Light: return BytesView(s_font_Roboto_Light, 66156); break;
		case SystemFontName::Roboto_LightItalic: return BytesView(s_font_Roboto_LightItalic, 72652); break;
		case SystemFontName::Roboto_Medium: return BytesView(s_font_Roboto_Medium, 66724); break;
		case SystemFontName::Roboto_MediumItalic: return BytesView(s_font_Roboto_MediumItalic, 73364); break;
		case SystemFontName::Roboto_Regular: return BytesView(s_font_Roboto_Regular, 65388); break;
		case SystemFontName::Roboto_Thin: return BytesView(s_font_Roboto_Thin, 65324); break;
		case SystemFontName::Roboto_ThinItalic: return BytesView(s_font_Roboto_ThinItalic, 70860); break;
		case SystemFontName::RobotoCondensed_Bold: return BytesView(s_font_RobotoCondensed_Bold, 66488); break;
		case SystemFontName::RobotoCondensed_BoldItalic: return BytesView(s_font_RobotoCondensed_BoldItalic, 74004); break;
		case SystemFontName::RobotoCondensed_Italic: return BytesView(s_font_RobotoCondensed_Italic, 72808); break;
		case SystemFontName::RobotoCondensed_Light: return BytesView(s_font_RobotoCondensed_Light, 65540); break;
		case SystemFontName::RobotoCondensed_LightItalic: return BytesView(s_font_RobotoCondensed_LightItalic, 73228); break;
		case SystemFontName::RobotoCondensed_Regular: return BytesView(s_font_RobotoCondensed_Regular, 65028); break;
	}

	return BytesView();
}

NS_MD_END
