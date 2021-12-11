// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"

#include "STContinueToken.cc"
#include "STInputFile.cc"
#include "STStorageAdapter.cc"
#include "STStorageAuth.cc"
#include "STStorageField.cc"
#include "STStorageFile.cc"
#include "STStorageInternals.cc"
#include "STStorageObject.cc"
#include "STStorageQuery.cc"
#include "STStorageQueryList.cc"
#include "STStorageTransaction.cc"
#include "STStorageUser.cc"
#include "STStorageWorker.cc"
#include "STStorageScheme.cc"

#include "STSqlDriver.cc"
#include "STSqlHandle.cc"
#include "STSqlHandleObject.cc"
#include "STSqlHandleProp.cc"
#include "STSqlQuery.cc"
#include "STSqlResult.cc"

#include "STPqHandle.cc"
#include "STPqHandleInit.cc"
#include "STPqDriver.cc"

#include "STSqliteHandle.cc"
#include "STSqliteHandleInit.cc"
#include "STSqliteDriver.cc"

#include "STFieldIntArray.cc"
#include "STFieldPoint.cc"
