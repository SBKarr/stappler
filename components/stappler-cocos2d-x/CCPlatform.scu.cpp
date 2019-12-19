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

#include "platform/CCFileUtils.cpp"
#include "platform/CCGLView.cpp"
#include "platform/CCImage.cpp"

#include "platform/desktop/CCGLViewImpl-desktop.cpp"
#include "platform/linux/CCStdC-linux.cpp"
#include "platform/linux/CCFileUtils-linux.cpp"
#include "platform/linux/CCDevice-linux.cpp"
#include "platform/linux/CCApplication-linux.cpp"
#include "platform/linux/CCCommon-linux.cpp"

#include "platform/android/CCApplication-android.cpp"
#include "platform/android/CCCommon-android.cpp"
#include "platform/android/CCDevice-android.cpp"
#include "platform/android/CCGLViewImpl-android.cpp"
#include "platform/android/CCFileUtils-android.cpp"
#include "platform/android/javaactivity-android.cpp"
#include "platform/android/jni/DPIJni.cpp"
#include "platform/android/jni/IMEJni.cpp"
#include "platform/android/jni/Java_org_cocos2dx_lib_Cocos2dxBitmap.cpp"
#include "platform/android/jni/Java_org_cocos2dx_lib_Cocos2dxHelper.cpp"
#include "platform/android/jni/Java_org_cocos2dx_lib_Cocos2dxRenderer.cpp"
#include "platform/android/jni/JniHelper.cpp"
#include "platform/android/jni/TouchesJni.cpp"

#include "platform/win32/CCApplication-win32.cpp"
#include "platform/win32/CCDevice-win32.cpp"
#include "platform/win32/CCStdC-win32.cpp"
#include "platform/win32/CCCommon-win32.cpp"
#include "platform/win32/CCFileUtils-win32.cpp"
