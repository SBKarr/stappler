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

#include "SPDefine.h"

#if CC_SPRITEBATCHNODE_RENDER_SUBPIXEL
#define RENDER_IN_SUBPIXEL
#else
#define RENDER_IN_SUBPIXEL
//#define RENDER_IN_SUBPIXEL(__ARGS__) (ceil(__ARGS__))
#endif

#include "dynamic_atlas/SPDynamicAtlas.cc"
#include "dynamic_atlas/SPDynamicBatchCommand.cc"
#include "dynamic_atlas/SPDynamicQuadArray.cc"
#include "dynamic_atlas/SPDynamicQuadAtlas.cc"
#include "dynamic_atlas/SPDynamicTriangleArray.cc"
#include "dynamic_atlas/SPDynamicTriangleAtlas.cc"
#include "dynamic_atlas/SPDynamicLinearGradient.cc"
#include "dynamic_atlas/SPStencilCache.cc"
#include "locale/SPLocale.cc"
#include "resources/SPFont.cc"
#include "resources/SPResource.cc"
#include "storekit/SPStoreKit.cc"
#include "storekit/SPStoreProduct.cc"
#include "texture/SPGLProgramSet.cc"
#include "texture/SPTextureCache.cc"
