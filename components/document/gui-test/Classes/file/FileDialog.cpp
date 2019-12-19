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
#include "FileDialog.h"
#include "SPPlatform.h"
#include "SPThread.h"

#if MSYS
#include <windows.h>
#include <shlobj.h>
#endif

NS_SP_EXT_BEGIN(app)

#if MSYS
namespace dialog {
	static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM lpData) {
		if (uMsg == BFFM_INITIALIZED) {
			String tmp = string::toUtf8((const char16_t *) lpData);
			std::cout << "path: " << tmp << std::endl;
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		}
		return 0;
	}

	void open(const String &saved, const Function<void(const String &)> &func, Mode m) {
		TCHAR path[MAX_PATH];

		auto wsaved = string::toUtf16(filesystem_native::posixToNative(saved));

		BROWSEINFO bi = { 0 };
		bi.lpszTitle  = (L"Browse for folder...");
		bi.ulFlags    = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		bi.lpfn       = BrowseCallbackProc;
		bi.lParam     = (LPARAM) wsaved.data();

		LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );

		if (pidl != 0) {
			//get the name of the folder and put it in path
			SHGetPathFromIDList ( pidl, path );

			//free memory used
			IMalloc * imalloc = 0;
			if (SUCCEEDED( SHGetMalloc ( &imalloc ))) {
				imalloc->Free ( pidl );
				imalloc->Release ( );
			}

			func(filesystem_native::nativeToPosix(string::toUtf8((const char16_t *)path)));
		}
	}
}
#endif

#if LINUX
namespace dialog {
	bool _dialogOpened = false;
	Thread dialogThread("Linux.DialogThread");

	void open(const std::string &path, const std::function<void(const std::string &)> &func, Mode m) {
		if (_dialogOpened) {
			func("");
			return;
		}

		_dialogOpened = true;
		auto ret = new std::string();
		dialogThread.perform([ret, path, m] (const thread::Task &) -> bool {
			std::string cmd("zenity --file-selection");
			if (m == Mode::SelectDir) {
				cmd.append(" --directory");
			}
			if (!path.empty()) {
				cmd.append(" --filename=").append(path);
			}

			char buf[1024] = { 0 };
			FILE *f = popen(cmd.c_str(), "r");
			if (f) {
				auto fpath = fgets(buf, 1024, f);
				if (fpath) {
					(*ret) = fpath;
					string::trim((*ret));
				}
				pclose(f);
			}

			return true;
		}, [ret, func] (const thread::Task &, bool success) {
			func(*ret);
			delete ret;
			_dialogOpened = false;
		});
	}
}
#endif

NS_SP_EXT_END(app)
