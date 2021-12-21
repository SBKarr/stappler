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

#include "SPLayout.h"

#include "SLStyle.cc"
#include "SLStyleColors.cc"
#include "SLStyleCompiled.cc"
#include "SLStyleMedia.cc"
#include "SLStyleValues.cc"
#include "SLParser.cc"
#include "SLReader.cc"
#include "SLNode.cc"
#include "SLRendererTypes.cc"

#include "SLColor.cc"
#include "SLGeometry.cc"
#include "SLMat4.cc"
#include "SLMathUtil.cc"
#include "SLQuaternion.cc"
#include "SLVec2.cc"
#include "SLVec3.cc"
#include "SLVec4.cc"

#include "SLDraw.cc"
#include "SLPath.cc"
#include "SLImage.cc"

SP_EXTERN_C void tessLog(const char *msg) {
	stappler::log::text("Tess", msg);
}

