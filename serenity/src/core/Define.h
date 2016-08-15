/*
   Copyright 2013 Roman "SBKarr" Katuntsev, LLC St.Appler

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef SADEFINES_H
#define	SADEFINES_H

#include "Config.h"

#include "SPAprArray.h"
#include "SPAprTable.h"
#include "SPAprUri.h"
#include "SPAprUuid.h"

#include "SPData.h"

NS_SA_BEGIN

struct InputConfig {
	enum class Require {
		None = 0,
		Data = 1,
		Files = 2,
		Body = 4,
		FilesAsData = 8,
	};

	InputConfig() = default;
	InputConfig(const InputConfig &) = default;
	InputConfig & operator=(const InputConfig &) = default;

	InputConfig(InputConfig &&) = default;
	InputConfig & operator=(InputConfig &&) = default;

	Require required = Require::None;
	size_t maxRequestSize = config::getMaxRequestSize();
	size_t maxVarSize = config::getMaxVarSize();
	size_t maxFileSize = config::getMaxFileSize();

	TimeInterval updateTime = config::getInputUpdateTime();
	float updateFrequency = config::getInputUpdateFrequency();
};

SP_DEFINE_ENUM_AS_MASK(InputConfig::Require);

constexpr apr_time_t operator"" _apr_sec ( unsigned long long int val ) { return val * 1000000; }
constexpr apr_time_t operator"" _apr_msec ( unsigned long long int val ) { return val * 1000; }
constexpr apr_time_t operator"" _apr_mksec ( unsigned long long int val ) { return val; }

NS_SA_END

#endif	/* SADEFINES_H */

