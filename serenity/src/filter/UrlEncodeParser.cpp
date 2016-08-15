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

#include "Define.h"
#include "SPCharReader.h"
#include "UrlEncodeParser.h"

NS_SA_BEGIN

UrlEncodeParser::UrlEncodeParser(const InputConfig &c, size_t s) : InputParser(c, s) { }
UrlEncodeParser::~UrlEncodeParser() { }

void UrlEncodeParser::run(const char *str, size_t len) {
	basicParser.read((const uint8_t *)str, len);
}

void UrlEncodeParser::finalize() { }

NS_SA_END
