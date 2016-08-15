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
void generateFontSet(FontSet::Config &&, const FontSet::Callback &callback);

void acquireFontAsset(const std::set<std::string> &urls, const std::function<void(const std::vector<Asset *> &)> &);

std::set<std::string> getConfigUrls(const FontSet::Config &);
std::set<std::string> getRequestsUrls(const std::vector<FontRequest> &);
std::set<std::string> getRequestUrls(const FontRequest &);
std::set<std::string> getReceiptUrls(const FontRequest::Receipt &);
bool isReceiptUrl(const std::string &);

Thread &thread();

uint64_t getReceiptHash(const FontRequest::Receipt &r,  const cocos2d::Map<std::string, Asset *> &assets);

void setFallbackFont(const std::string &);
const std::string &getFallbackFont();

NS_SP_EXT_END(resource)

#endif /* LIBS_STAPPLER_FEATURES_RESOURCES_SPRESOURCELIBRARY_H_ */
