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

#include "base/CCScheduler.cpp"
#include "base/CCProfiling.cpp"
#include "base/CCDirector.cpp"
#include "base/CCAutoreleasePool.cpp"
#include "base/CCRef.cpp"
#include "base/CCTouch.cpp"
#include "base/atitc.cpp"
#include "base/etc1.cpp"
#include "base/pvr.cpp"
#include "base/s3tc.cpp"
#include "base/ZipUtils.cpp"
#include "base/ccTypes.cpp"
#include "base/ccRandom.cpp"
#include "base/ccUtils.cpp"
#include "base/ccCArray.cpp"
#include "base/CCConsole.cpp"
#include "base/CCConfiguration.cpp"
#include "base/CCEventDispatcher.cpp"
#include "base/CCEventListener.cpp"
#include "base/CCEventListenerCustom.cpp"
#include "base/CCEventListenerFocus.cpp"
#include "base/CCEventListenerKeyboard.cpp"
#include "base/CCEventListenerMouse.cpp"
#include "base/CCEventListenerTouch.cpp"
#include "base/CCEvent.cpp"
#include "base/CCEventCustom.cpp"
#include "base/CCEventFocus.cpp"
#include "base/CCEventKeyboard.cpp"
#include "base/CCEventMouse.cpp"
#include "base/CCEventTouch.cpp"

