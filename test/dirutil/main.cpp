
#include "SPFilesystem.h"
#include "SPData.h"
#include "SPDataStream.h"

#include <string>
#include <unistd.h>
#include <iostream>
#include <fstream>

#define HELP_STRING \
	"usage: sptool <command> <address> <config-file>\n" \
	"where command is one of:\n" \
	"	send - send request to server\n" \
	"	auth - test authorization system\n" \
	"	repack - repack json as cbor\n" \

using namespace stappler;

int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'e') {
		ret.setBool(true, "emulate");
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

	data::Value opts = data::parseCommandLineOptions(argc, argv,
			&parseOptionSwitch, &parseOptionString);
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		std::cout << " Current work dir: " << stappler::filesystem::currentDir() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPath() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPath() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePath() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts << "\n";
		return 0;
	};

	auto args = opts.getValue("args");
	if (args.size() >= 2) {
		String arg = args.getString(1);
		if (arg == "ls") {
			if (args.size() == 2) {
				filesystem::ftw(filesystem::currentDir(), [] (const String &path, bool isFile) {
					std::cout << path << "\n";
				}, 1, true);
			} else {
				auto path = args.getString(2);
				filesystem::ftw(filesystem::currentDir(path), [] (const String &path, bool isFile) {
					std::cout << path << "\n";
				}, 1, true);
			}
		}
	}

	return 0;
}
