// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPData.h"
#include "SPThreadManager.h"
#include "SPData.h"
#include "SPNetworkDataTask.h"
#include "SPDataStream.h"
#include "SLPath.h"
#include "SLImage.h"

#include <string>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "SPFilesystem.h"

NS_SP_EXT_BEGIN(app)

static Map<String, String> s_extraIconSources {
	std::make_pair("stappler_fav_icon", "path://M24,34.54,36.36,42,33.08,27.94,44,18.48,29.62,17.26,24,4,18.38,17.26,4,18.48,14.92,27.94,11.64,42z"),
	std::make_pair("stappler_fav_outline", "path://M44,18.48,29.62,17.24,24,4,18.38,17.26,4,18.48,14.92,27.94,11.64,42,24,34.54,36.36,42,33.1,27.94,44,18.48zm-20,12.32-7.52,4.54,2-8.56-6.64-5.76,8.76-0.76,3.4-8.06,3.42,8.08,8.76,0.76-6.64,5.76,2,8.56l-7.54-4.56z"),
	std::make_pair("stappler_text_format_100", "path://m10 39.04h4v4h-4zm0-8v4h28v-4zm9-8.4h10l1.8 4.4h4.2l-9.5-22h-3l-9.5 22h4.2zm5-13.64l3.74 10.04h-7.48z"),
	std::make_pair("stappler_text_format_125", "path://m16 39.04h4v4h-4zm-6 0h4v4h-4zm0-8v4h28v-4zm9-8.4h10l1.8 4.4h4.2l-9.5-22h-3l-9.5 22h4.2zm5-13.64l3.74 10.04h-7.48z"),
	std::make_pair("stappler_text_format_150", "path://m22 39.04h4v4h-4zm-6 0h4v4h-4zm-6 0h4v4h-4zm0-8v4h28v-4zm9-8.4h10l1.8 4.4h4.2l-9.5-22h-3l-9.5 22h4.2zm5-13.64l3.74 10.04h-7.48z"),
	std::make_pair("stappler_text_format_175", "path://m28 39.04h4v4h-4zm-6 0h4v4h-4zm-6 0h4v4h-4zm-6 0h4v4h-4zm0-8v4h28v-4zm9-8.4h10l1.8 4.4h4.2l-9.5-22h-3l-9.5 22h4.2zm5-13.64l3.74 10.04h-7.48z"),
	std::make_pair("stappler_text_format_200", "path://m34 39.04h4v4h-4zm-6 0h4v4h-4zm-6 0h4v4h-4zm-6 0h4v4h-4zm-6 0h4v4h-4zm0-8v4h28v-4h-28zm9-8.4h10l1.8 4.4h4.2l-9.5-22h-3l-9.5 22h4.2l1.8-4.4zm5-13.64l3.74 10.04h-7.48l3.74-10.04z"),
	std::make_pair("stappler_layout_vertical", "path://m38 11h3v26h-3zm-30 26h28v-12h-28zm0-26v12h28v-12z"),
	std::make_pair("stappler_layout_horizontal", "path://m8 39v-3h32v3zm0-5h15v-24h-15zm17-24v24h15v-24z"),
};

struct IconInfo {
	String name;
	Bytes data;
	String title;
};

void savePath(IconInfo &ic, const layout::Path &path) {
	ic.data = path.encode();

	ic.title = ic.name;
	ic.title[0] = toupper(ic.title[0]);
}

void readIcon(const StringView &name, const StringView &path, Map<String, stappler::app::IconInfo> &icons) {
	auto iconsIt = icons.emplace(name.str(), IconInfo{name.str()}).first;
	layout::Image img;
	if (img.init(FilePath(path))) {
		auto &paths = img.getPaths();
		if (paths.size() > 1) {
			layout::Path rootPath;
			for (auto &it : paths) {
				rootPath.addPath(it.second);
			}
			savePath(iconsIt->second, rootPath);
		} else if (!paths.empty()) {
			savePath(iconsIt->second, paths.begin()->second);
		}
	}
}

void readMaterialIconsDir(const StringView &dirPath, Map<String, stappler::app::IconInfo> &icons) {
	filesystem::ftw(dirPath, [&] (const StringView &path, bool isFile) {
		if (!isFile) {
			auto nextPath = toString(path, "/svg/production");
			if (filesystem::exists(nextPath)) {
				filesystem::ftw(nextPath, [&] (const StringView &icPath, bool isFile) {
					if (isFile && icPath.ends_with("48px.svg")) {
						auto iconName = filepath::name(icPath);
						if (iconName.starts_with("ic_")) {
							iconName += "ic_"_len;
						}
						if (iconName.ends_with("_48px")) {
							iconName = StringView(iconName.data(), iconName.size() - "_48px"_len);
						}
						readIcon(toString(filepath::lastComponent(path), "_", iconName), icPath, icons);
					}
				});
			}
		}
	}, 1);

	for (auto &it : s_extraIconSources) {
		layout::Path path;
		if (it.second.compare(0, 7, "path://") == 0) {
			if (path.init(it.second.substr(7))) {

				auto title = it.first;
				title[0] = toupper(title[0]);

				icons.emplace(it.first, IconInfo{it.first, path.encode(), title}).first;
			}
		}
	}
}

NS_SP_EXT_END(app)

using namespace stappler;

auto HELP_STRING =
R"Text(pathgen <path-to-material-icons-repo>
	- generate material icons sources and header for stappler vg engine
)Text";

auto LICENSE_STRING =
R"Text(/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

// Generated with PathGenerator (%STAPPLER_ROOT%/test/generators/PathGenerator)
// from material-design-icons repo: https://github.com/google/material-design-icons

)Text";

int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	}
	return 1;
}

int parseOptionString(data::Value &ret, const String &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	}
	return 1;
}

void sp_android_terminate () {
	stappler::log::text("Exception", "Crash on exceprion");
  __gnu_cxx::__verbose_terminate_handler();
}

int _spMain(argc, argv) {
	std::set_terminate(sp_android_terminate);

	data::Value opts = data::parseCommandLineOptions(argc, argv, &parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	};

	Map<String, stappler::app::IconInfo> icons;

	auto &args = opts.getValue("args");
	if (args.size() > 1) {
		auto path = args.getString(1);
		stappler::app::readMaterialIconsDir(path, icons);
	}


	StringStream sourceFile;
	StringStream headerFile;

	sourceFile << LICENSE_STRING <<
R"Text(
#include "MaterialIconStorage.h"

NS_MD_BEGIN
)Text";

	for (auto &it : icons) {
		layout::Path path;
		auto text = base16::encode(CoderSource(it.second.data));
		auto d = text.c_str();
		bool first = false;
		sourceFile << "static const uint8_t s_icon_" << it.first << "[] = { ";
		for (size_t i = 0; i < text.size() / 2; ++ i) {
			if (!first) {
				first = true;
			} else {
				sourceFile << ",";
			}
			sourceFile << "0x" << d[i * 2] << d[i * 2 + 1];
		}
		sourceFile << "};\n";
	}

	sourceFile << "\nstatic struct IconDataStruct { IconName name; size_t len; const uint8_t *data; } s_iconTable["
			<< icons.size() << "] = {\n";

	size_t i = 0;
	for (auto &it : icons) {
		sourceFile << "\t{ IconName::" << it.second.title << ", " << it.second.data.size() << ", s_icon_" << it.first << " },\n";
		++ i;
	}

	sourceFile << "};\n\nstatic uint16_t s_iconCount = " << icons.size() << ";\n\n";

	sourceFile <<
R"Text(
NS_MD_END
)Text";

	headerFile << LICENSE_STRING <<
R"Text(
#ifndef MATERIAL_RESOURCES_MATERIALICONNAMES_H_
#define MATERIAL_RESOURCES_MATERIALICONNAMES_H_

#include "Material.h"

NS_MD_BEGIN

enum class IconName : uint16_t {
	None = 0,
	Empty,

	Dynamic_Navigation,
	Dynamic_Loader,
	Dynamic_Expand,

)Text";
	for (auto &it : icons) {
		headerFile << "\t" << it.second.title << ",\n";
	}

	headerFile <<
R"Text(};

NS_MD_END

#endif /* MATERIAL_RESOURCES_MATERIALICONNAMES_H_ */
)Text";

	auto dir = filesystem::currentDir("gen");
	filesystem::mkdir(dir);

	auto sourceFilePath = toString(dir, "/MaterialIconSources.cc");
	auto headerFilePath = toString(dir, "/MaterialIconNames.h");

	filesystem::remove(sourceFilePath);;
	filesystem::write(sourceFilePath, sourceFile.str());

	filesystem::remove(headerFilePath);
	filesystem::write(headerFilePath, headerFile.str());

	std::cout << icons.size() << " was processed\n";
	std::cout << "Source file: " << sourceFilePath << "\n"
			<< "Header file: " << headerFilePath << "\n";

	return 0;
}
