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

#ifndef LIBS_STAPPLER_FEATURES_RESOURCES_SPRESOURCELIBRARY_H_
#define LIBS_STAPPLER_FEATURES_RESOURCES_SPRESOURCELIBRARY_H_

#include "SPDefine.h"
#include "SPIcon.h"
#include "SPAsset.h"

NS_SP_EXT_BEGIN(resource)

void generateIconSet(IconSet::Config &&, const IconSet::Callback &callback);

void acquireFontAsset(const Set<String> &urls, const Function<void(const Vector<Rc<Asset>> &)> &);

bool isReceiptUrl(const String &);

Thread &thread();

void setFallbackFont(const String &);
const String &getFallbackFont();

NS_SP_EXT_END(resource)

#endif /* LIBS_STAPPLER_FEATURES_RESOURCES_SPRESOURCELIBRARY_H_ */
