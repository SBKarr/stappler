// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPFont.h"

#include "SPString.h"
#include "SPThread.h"
#include "SPData.h"
#include "SPAsset.h"
#include "SPAssetLibrary.h"
#include "SPResource.h"
#include "SPFilesystem.h"

#include "SPFallbackFont-DejaVuSansStappler.cc"

NS_SP_EXT_BEGIN(resource)

static Thread s_resourceThread("ResourceLibraryThread");

std::string getFontAssetPath(const std::string &url) {
	std::string dir = filesystem::cachesPath("font_assets");
	return toString(dir, "/", string::stdlibHashUnsigned(url), ".asset");
}

void acquireFontAsset(const Set<String> &urls, const Function<void(const Vector<Rc<Asset>> &)> &cb) {
	filesystem::mkdir(filesystem::cachesPath("font_assets"));
	Vector<AssetLibrary::AssetRequest> vec;
	for (auto &it : urls) {
		vec.push_back(AssetLibrary::AssetRequest(nullptr, it, getFontAssetPath(it)));
	}
	AssetLibrary::getInstance()->getAssets(vec, cb);
}


bool isReceiptUrl(const std::string &str) {
	return str.compare(0, 7, "http://") == 0 || str.compare(0, 8, "https://") == 0
			|| str.compare(0, 6, "ftp://") == 0 || str.compare(0, 7, "ftps://") == 0;
}

Thread &thread() {
	return s_resourceThread;
}

static std::mutex s_fallbackFontMutex;
static BytesView s_fallbackFontBytes(s_font_DejaVuSansStappler, 376604);

BytesView extern_font_DejaVuSansStappler(s_font_DejaVuSansStappler, 376604);

void setFallbackFont(BytesView v) {
	s_fallbackFontMutex.lock();
	s_fallbackFontBytes = v;
	s_fallbackFontMutex.unlock();
}

BytesView getFallbackFont() {
	BytesView ret;
	s_fallbackFontMutex.lock();
	ret = s_fallbackFontBytes;
	s_fallbackFontMutex.unlock();
	return ret;
}

NS_SP_EXT_END(resource)
