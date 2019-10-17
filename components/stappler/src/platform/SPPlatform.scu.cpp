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
#include "android/SPClipboard.cc"
#include "android/SPDevice.cc"
#include "android/SPFilesystem.cc"
#include "android/SPIME.cc"
#include "android/SPInteraction.cc"
#include "android/SPJNI.cc"
#include "android/SPNetwork.cc"
#include "android/SPProc.cc"
#include "android/SPRender.cc"
#include "android/SPStatusBar.cc"
#include "android/SPStoreKit.cc"

#include "linux/SPClipboard.cc"
#include "linux/SPDevice.cc"
#include "linux/SPIME.cc"
#include "linux/SPInteraction.cc"
#include "linux/SPNetwork.cc"
#include "linux/SPRender.cc"
#include "linux/SPStatusBar.cc"
#include "linux/SPStoreKit.cc"

#include "win32/SPClipboard.cc"
#include "win32/SPDevice.cc"
#include "win32/SPIME.cc"
#include "win32/SPInteraction.cc"
#include "win32/SPNetwork.cc"
#include "win32/SPRender.cc"
#include "win32/SPStatusBar.cc"
#include "win32/SPStoreKit.cc"

#include "universal/SPApplication.cc"
#include "universal/SPDevice.cc"
#include "universal/SPScreenOrientation.cc"

#ifndef SP_RESTRICT
#include "universal/SPScreen.cc"
#endif

#include "ios/SPReceiptValidation.cc"
