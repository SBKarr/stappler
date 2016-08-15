#include "SPData.h"
#include "SPFilesystem.h"
#include "SPBitmap.h"

#include <string>
#include <unistd.h>
#include <iostream>

#define HELP_STRING \
	"usage: spimagesize <path>\n" \

USING_NS_SP;

int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	}
	return 1;
}

int parseOptionString(data::Value &ret, const std::string &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	}
	return 1;
}

void sp_android_terminate () {
	stappler::log("Crash on exceprion");
  __gnu_cxx::__verbose_terminate_handler();
}

int main(int argc, char **argv) {
	std::set_terminate(sp_android_terminate);

	auto opts = data::parseCommandLineOptions(argc, (const char **)argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	};

	auto &args = opts.getValue("args");
	if (args.size() < 2) {
		std::cout << "At least 1 argument is required!\n";
		return -1;
	}

	auto path = args.getString(1);
	if (!path.empty()) {
		path = filesystem::currentDir(path);
		filesystem::ftw(path, [] (const String &path, bool isFile) {
			if (isFile) {
				size_t width = 0, height = 0;
				if (Bitmap::getImageSize(path, width, height)) {
					std::cout << path << " - " << width << "x" << height << "\n";
				} else {
					std::cout << path << " - not image\n";
				}
			}
		}, 1, false);
	}

	return 0;
}
