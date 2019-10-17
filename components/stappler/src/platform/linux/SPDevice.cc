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
#include "platform/CCDevice.h"

#include "SPPlatform.h"
#include "SPScreenOrientation.h"

#include "math/CCGeometry.h"

#if (LINUX)

#ifndef SP_RESTRICT

namespace stappler::platform::desktop {
	void setScreenSize(const Size &size);
	Size getScreenSize();
	bool isTablet();
	String getPackageName();
	float getDensity();
	String getUserLanguage();
	String getAppVersion();
}

#endif

namespace stappler::platform::device {
	bool _isTablet() {
#ifndef SP_RESTRICT
		return desktop::isTablet();
#else
		return true;
#endif
	}
	String _userAgent() {
		return "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:41.0)";
	}
	String _deviceIdentifier() {
		auto path = platform::filesystem::_getCachesPath();
		auto devIdPath = path + "/.devid";
		if (stappler::filesystem::exists(devIdPath)) {
			auto data = stappler::filesystem::readIntoMemory(devIdPath);
			return base16::encode(data);
		} else {
			Bytes data; data.resize(16);
			auto fp = fopen("/dev/urandom", "r");
			if (fread(data.data(), 1, data.size(), fp) == 0) {
				log::text("Device", "Fail to read random bytes");
			}
			fclose(fp);

			stappler::filesystem::write(devIdPath, data);
			return base16::encode(data);
		}
	}
	String _bundleName() {
#ifndef SP_RESTRICT
		return desktop::getPackageName();
#else
		return "org.stappler.stappler";
#endif
	}
	String _userLanguage() {
#ifndef SP_RESTRICT
		return desktop::getUserLanguage();
#else
		return "en-us";
#endif
	}
	String _applicationName() {
		return "Linux App";
	}
	String _applicationVersion() {
#ifndef SP_RESTRICT
		return desktop::getAppVersion();
#else
		return "1.0";
#endif
	}
	Size _screenSize() {
#ifndef SP_RESTRICT
		auto size =  desktop::getScreenSize();
		if (size.width <= size.height) {
			return size;
		} else {
			return Size(size.height, size.width);
		}
#else
		return Size(1024, 768);
#endif
	}
	Size _viewportSize(const Size &screenSize, bool isTablet) {
#ifndef SP_RESTRICT
		auto size =  desktop::getScreenSize();
		if (size.width <= size.height) {
			return size;
		} else {
			return Size(size.height, size.width);
		}
#else
		return Size(1024, 768);
#endif
	}
	float _designScale(const Size &screenSize, bool isTablet) {
		return 1.0f;
	}
	const ScreenOrientation &_currentDeviceOrientation() {
#ifndef SP_RESTRICT
		auto size = desktop::getScreenSize();
		if (size.width > size.height) {
			return ScreenOrientation::LandscapeLeft;
		} else {
			return ScreenOrientation::PortraitTop;
		}
#else
		return ScreenOrientation::LandscapeLeft;
#endif
	}
	Pair<uint64_t, uint64_t> _diskSpace() {
		uint64_t totalSpace = 0;
		uint64_t totalFreeSpace = 0;

		return pair(totalSpace, totalFreeSpace);
	}
	int _dpi() {
#ifdef SP_RESTRICT
		return 92;
#else
		return 92 * desktop::getDensity();
#endif
	}

	void _onDirectorStarted() { }
	float _density() {
#ifdef SP_RESTRICT
		return 1.0f;
#else
		return desktop::getDensity();
#endif
	}
}

#endif
