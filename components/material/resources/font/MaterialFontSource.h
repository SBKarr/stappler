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

NS_MD_BEGIN

enum class SystemFontName {
	DejaVuSansMono,
	DejaVuSansMono_Bold,
	DejaVuSansStappler,
	Roboto_Black,
	Roboto_BlackItalic,
	Roboto_Bold,
	Roboto_BoldItalic,
	Roboto_Italic,
	Roboto_Light,
	Roboto_LightItalic,
	Roboto_Medium,
	Roboto_MediumItalic,
	Roboto_Regular,
	Roboto_Thin,
	Roboto_ThinItalic,
	RobotoCondensed_Bold,
	RobotoCondensed_BoldItalic,
	RobotoCondensed_Italic,
	RobotoCondensed_Light,
	RobotoCondensed_LightItalic,
	RobotoCondensed_Regular
};

BytesView getSystemFont(SystemFontName);

NS_MD_END
