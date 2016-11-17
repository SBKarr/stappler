/*
 * SPResourceLibrary.h
 *
 *  Created on: 12 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RESOURCES_SPRESOURCELIBRARY_H_
#define LIBS_STAPPLER_FEATURES_RESOURCES_SPRESOURCELIBRARY_H_

#include "SPDefine.h"
#include "SPFont.h"
#include "SPIcon.h"

NS_SP_EXT_BEGIN(resource)

void generateIconSet(IconSet::Config &&, const IconSet::Callback &callback);

void acquireFontAsset(const Set<String> &urls, const Function<void(const Vector<Asset *> &)> &);

bool isReceiptUrl(const String &);

Thread &thread();

void setFallbackFont(const std::string &);
const std::string &getFallbackFont();

NS_SP_EXT_END(resource)

#endif /* LIBS_STAPPLER_FEATURES_RESOURCES_SPRESOURCELIBRARY_H_ */
