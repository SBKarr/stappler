#ifndef SP_RESTRICT

#include "SPDefine.h"

#include "SPData.h"
#include "SPScreen.h"
#include "SPPlatform.h"

#include "platform/CCApplication.h"

NS_SP_PLATFORM_BEGIN

namespace desktop {
	cocos2d::Size _screenSize;
	bool _isTablet = false;
	bool _isFixed = false;
	std::string _package;
	std::string _userLanguage;
	float _density = 1.0f;

	void setScreenSize(const cocos2d::Size &size) { _screenSize = size; }
	cocos2d::Size getScreenSize() { return _screenSize; }
	bool isTablet() { return _isTablet; }
	bool isFixed() { return _isFixed; }
	std::string getPackageName() { return _package; }
	float getDensity() { return _density; }
	std::string getUserLanguage() { return _userLanguage; }
}

NS_SP_PLATFORM_END

USING_NS_CC;

int parseOptionSwitch(stappler::data::Value &ret, char c, const char *str) {
	return 1;
}

int parseOptionString(stappler::data::Value &ret, const std::string &str,
					  int argc, const char * argv[]) {
	if (str.compare(0, 2, "w=") == 0) {
		auto s = std::stoi( str.substr(2) );
		if (s > 0) {
			ret.setInteger(s, "width");
		}
	} else if (str.compare(0, 2, "h=") == 0) {
		auto s = std::stoi( str.substr(2) );
		if (s > 0) {
			ret.setInteger(s, "height");
		}
	} else if (str.compare(0, 2, "d=") == 0) {
		auto s = std::stod( str.substr(2) );
		if (s > 0) {
			ret.setDouble(s, "density");
		}
	} else if (str.compare(0, 2, "l=") == 0) {
		ret.setString(str.substr(2), "locale");
	} else if (str == "tablet") {
		ret.setBool(true, "isTablet");
	} else if (str == "phone") {
		ret.setBool(false, "isTablet");
	} else if (str == "package") {
		ret.setString(argv[0], "package");
	} else if (str == "fixed") {
		ret.setBool(true, "fixed");
	}
	return 1;
}

stappler::data::Value parseOptions(int argc, const char * argv[]) {
	if (argc == 0) {
		return stappler::data::Value();
	}

	stappler::data::Value ret;
	auto &args = ret.setValue(stappler::data::Value(stappler::data::Value::Type::ARRAY), "args");

	int i = argc;
	while (i > 0) {
		const char *value = argv[argc - i];
		if (value[0] == '-') {
			if (value[1] == '-') {
				i -= (parseOptionString(ret, &value[2], i - 1, &argv[argc - i + 1]) - 1);
			} else {
				const char *str = &value[1];
				while (str[0] != 0) {
					str += parseOptionSwitch(ret, str[0], &str[1]);
				}
			}
		} else {
			args.addString(value);
		}
		i --;
	}

	return ret;
};

void sp_android_terminate () {
	stappler::log::text("Application", "Crash on exceprion");
  __gnu_cxx::__verbose_terminate_handler();
}

int main(int argc, char **argv)
{
	std::set_terminate(sp_android_terminate);
	std::string packageName = "org.stappler.stappler";
	stappler::data::Value val = stappler::data::readFile("app.json");
	if (val.isDictionary()) {
		stappler::data::Value &resultObj = val.getValue("result");
		if (resultObj.isArray()) {
			stappler::data::Value &obj = resultObj.getValue(0);
			if (obj.isDictionary()) {
				std::string name = obj.getString("alias");
				if (name.empty()) {
					name = obj.getString("name");
				}
				if (!name.empty()) {
					packageName = name;
				}
			}
		}
	}

	stappler::data::Value args = parseOptions(argc, (const char **)argv);
	if (args.getInteger("width") == 0) {
		args.setInteger(1024, "width");
	}

	if (args.getInteger("height") == 0) {
		args.setInteger(768, "height");
	}

	if (args.getString("package").empty()) {
		args.setString(packageName, "package");
	}

	if (args.getString("locale").empty()) {
		args.setString("en-us", "locale");
	}

	if (args.getDouble("density") == 0.0f) {
		args.setDouble(1.0f, "density");
	}

	stappler::platform::desktop::_screenSize = cocos2d::Size(args.getInteger("width"), args.getInteger("height"));
	stappler::platform::desktop::_isTablet = args.getBool("isTablet");
	stappler::platform::desktop::_package = args.getString("package");
	stappler::platform::desktop::_density = args.getDouble("density");
	stappler::platform::desktop::_isFixed = args.getBool("fixed");
	stappler::platform::desktop::_userLanguage = args.getString("locale");

    // create the application instance
    return Application::getInstance()->run();
}

#endif
