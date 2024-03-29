// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPCommon.h"
#include "SPFilesystem.cc"
#include "SPFilesystemNativeMingw.cc"
#include "SPFilesystemNativePosix.cc"
#include "SPHalfFloat.cc"
#include "SPHtmlParser.cc"
#include "SPNetworkHandle.cc"
#include "SPNetworkMultiHandle.cc"
#include "SPUrl.cc"
#include "SPUrlencodeParser.cc"
#include "SPMultipartParser.cc"
#include "SPTime.cc"
#include "SPTimeString.cc"
#include "SPLog.cc"
#include "SPDataSubscription.cc"
#include "SPRef.cc"
#include "SPSyncRWLock.cc"

#include "SPBitmap.cc"
#include "SPBitmapFormat.cc"
#include "SPBitmapResample.cc"
#include "SPSearchDistance.cc"
#include "SPSearchDistanceEdLib.cc"
#include "SPSearchUrl.cc"
#include "SPSearchParser.cc"
#include "SPSearchIndex.cc"
#include "SPSearchConfiguration.cc"
#include "SPSerenityPathQuery.cc"
#include "SPCrypto.cc"
#include "SPValid.cc"

#include "SPThreadTask.cc"
#include "SPThreadTaskQueue.cc"

#include "SPJsonWebToken.cc"
