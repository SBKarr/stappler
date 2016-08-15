#include "SPData.h"
#include "SPFilesystem.h"
#include "SPThreadManager.h"
#include "SPData.h"
#include "SPNetworkDataTask.h"
#include "SPDataStream.h"

#include <string>
#include <unistd.h>
#include <iostream>
#include <fstream>

NS_SP_EXT_BEGIN(app)

#define HELP_STRING \
	"usage: sptool <command> <address> <config-file>\n" \
	"where command is one of:\n" \
	"	send - send request to server\n" \
	"	auth - test authorization system\n" \
	"	repack - repack json as cbor\n" \

bool perform(const data::Value &opts, const String &host, const data::Value &file) {
	ThreadManager::getInstance()->setSingleThreaded(true);

	auto method = file.getString("method");
	auto query = file.getString("query");

	NetworkTask::Method m = NetworkTask::Method::Get;
	if (method == "POST") {
		m = NetworkTask::Method::Post;
	} else if (method == "PUT") {
		m = NetworkTask::Method::Put;
	} else if (method == "DELETE") {
		m = NetworkTask::Method::Delete;
	}

	auto url = host;
	if (!query.empty()) {
		url += "?" + query;
	}

	auto &data = file.getValue("data");

	auto t = Rc<NetworkDataTask>::create(m, url, data, data::EncodeFormat::Cbor);
	t->execute();

	auto retData = t->getData();

	std::cout << retData << "\n";

	return true;
}

bool auth(const data::Value &opts, const String &host, const data::Value &file) {
	auto name = file.getString("name");
	auto passwd = file.getString("passwd");
	size_t count = (size_t)file.getInteger("count");
	auto ttl = file.getInteger("maxAge");

	std::cout << "Authorize on " << host << "\n";
	std::cout << "\tName: '" << name << "'\n";
	std::cout << "\tPassword: '" << passwd << "'\n";
	std::cout << "\tTtl: " << ttl << "\n";

	auto url = toString(host, "/login?name=", string::urlencode(name), "&passwd=", string::urlencode(passwd), "&maxAge=", ttl);
	auto cookiesPath = filesystem::currentDir("network.cookies");
	std::cout << "\tUrl: " << url << "\n";
	std::cout << "\tCookies: " << cookiesPath << "\n";

	auto t = Rc<NetworkDataTask>::create(NetworkTask::Method::Get, url);
	t->getHandle().setCookieFile(cookiesPath);
	t->execute();

	auto retData = t->getData();

	std::cout << retData << "\n";
	std::cout.flush();

	for (size_t i = 0; i < count; i ++) {
		sleep(ttl / 2);

		if (retData.isDictionary() && retData.isString("token")) {
			std::cout << "\nUpdate on " << host << "\n";

			url = toString(host, "/update?token=", string::urlencode(retData.getString("token"), false), "&maxAge=", ttl);
			std::cout << "\tUrl: " << url << "\n";

			t = Rc<NetworkDataTask>::create(NetworkTask::Method::Get, url);
			t->getHandle().setCookieFile(cookiesPath);
			t->execute();

			auto retData = t->getData();

			std::cout << retData << "\n";
			std::cout.flush();
		}
	}

	if (retData.isDictionary() && retData.isString("token")) {
		std::cout << "\nCancel on " << host << "\n";

		url = toString(host, "/cancel?token=", string::urlencode(retData.getString("token"), false));
		std::cout << "\tUrl: " << url << "\n";

		t = Rc<NetworkDataTask>::create(NetworkTask::Method::Get, url);
		t->getHandle().setCookieFile(cookiesPath);
		t->execute();

		auto retData = t->getData();

		std::cout << retData << "\n\n";
		std::cout.flush();
	}

	return true;
}

NS_SP_EXT_END(app)

int parseOptionSwitch(stappler::data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'e') {
		ret.setBool(true, "emulate");
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
	if (args.size() < 3) {
		std::cout << "At least 3 argument is required!\n";
		return -1;
	}

	std::cout << stappler::data::EncodeFormat::Pretty;

	auto cmd = args.getString(1);
	auto address = args.getString(2);
	auto file = args.getString(3);

	if (cmd == "send") {
		auto data = stappler::data::readFile(stappler::filesystem::currentDir(file));
		if (data) {
			std::cout << "Request: " << data << "\n" << "Response: ";
			if (!opts.getBool("emulate")) {
				if (!stappler::app::perform(opts, address, data)) {
					std::cout << "failed!\n";
				}
			} else {
				std::cout << "false!\n";
			}
		} else {
			std::cout << "No file at " << file << ";\n";
			return -1;
		}
	} else if (cmd == "auth") {
		auto data = stappler::data::readFile(stappler::filesystem::currentDir(file));
		if (!stappler::app::auth(opts, address, data)) {
			return -1;
		}
	} else if (cmd == "repack") {
		auto data = stappler::data::readFile(stappler::filesystem::currentDir(address));

		stappler::filesystem::remove(stappler::filesystem::currentDir(file));
		std::ofstream f(stappler::filesystem::currentDir(file));
		stappler::data::write(f, data, stappler::data::EncodeFormat::Cbor);
		f.flush();
		f.close();
		return 0;
	} else if (cmd == "print") {
		auto data = stappler::data::readFile(stappler::filesystem::currentDir(address));
		std::cout << stappler::data::EncodeFormat::Pretty << data;
		return 0;
	}

	return 0;
}
