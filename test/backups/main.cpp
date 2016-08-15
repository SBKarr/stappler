/*
 * main.cpp
 *
 *  Created on: 23 марта 2016 г.
 *      Author: sbkarr
 */
#include "SPData.h"
#include "SPFilesystem.h"
#include <iostream>

#define HELP_STRING \
	"usage: sbaclup <command> <file>\n" \
	"where command is one of:\n" \
	"	split\n" \

NS_SP_EXT_BEGIN(app)

bool run(const data::Value &args, const data::Value &opts) {
	if (args.size() < 3) {
		std::cerr << "At least 2 argument is required!\n";
		return false;
	}

	bool pretty = opts.getBool("pretty");

	auto file = filesystem::currentDir(args.getString(2));
	if (!filesystem::exists(file)) {
		std::cerr << "No such file: " << file << "\n";
		return false;
	}

	auto name = filepath::name(file);
	auto path = filepath::root(file);

	auto &cmd = args.getString(1);
	if (cmd == "split") {
		auto data = data::readFile(file);
		if (data.isArray()) {
			size_t count = data.size();
			size_t i = 0;
			for (auto &it : data.asArray()) {
				if (it.hasValue("meta")) {
					auto prefix = it.getValue("meta").getString("prefix");
					std::cout << "-- " << prefix << " [" << i + 1 << "/" << count << "] ... ";
					std::cout.flush();

					data::Value val;
					val.addValue(std::move(it));

					auto savePath = filepath::merge(path, name + "." + prefix + ".json");
					filesystem::remove(savePath);
					data::save(val, savePath, (pretty?data::EncodeFormat::Pretty:data::EncodeFormat::Json));
					std::cout << "DONE (" << savePath << ")\n";
				}
				i++;
			}
		}

		return true;
	} else if (cmd == "erase") {
		auto data = data::readFile(file);
		if (data.isArray()) {
			size_t count = data.size();
			size_t i = 0;
			for (auto &it : data.asArray()) {
				if (it.hasValue("meta")) {
					auto prefix = it.getValue("meta").getString("prefix");
					std::cout << "-- " << prefix << " [" << i + 1 << "/" << count << "] ... ";
					std::cout.flush();

					if (prefix == "apps_application") {
						auto &objs = it.getValue("objects");
						for (auto &it : objs.asArray()) {
							it.erase("androiddevices");
							it.erase("iosdevices");
							it.erase("androiddevices_debug");
							it.erase("iosdevices_debug");
						}
					}
					std::cout << "DONE\n";
				}
				i++;
			}
		}

		auto savePath = filepath::merge(path, name + ".erased.json");
		data::save(data, savePath, (pretty?data::EncodeFormat::Pretty:data::EncodeFormat::Json));
	}

	return false;
}

NS_SP_EXT_END(app)

int parseOptionSwitch(stappler::data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'p') {
		ret.setBool(true, "pretty");
	}
	return 1;
}

int parseOptionString(stappler::data::Value &ret, const std::string &str, int argc, const char * argv[]) {
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

	auto opts = stappler::data::parseCommandLineOptions(argc, (const char **)argv,
			&parseOptionSwitch, &parseOptionString);

	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	};

	auto &args = opts.getValue("args");
	return (stappler::app::run(args, opts)?0:-1);
}

