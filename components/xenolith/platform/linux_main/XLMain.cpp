
#include "XLDefine.h"
#include "XLPlatform.h"
#include "XLApplication.h"

namespace stappler::xenolith::platform::desktop {
	Size _screenSize;
	bool _isTablet = false;
	bool _isFixed = false;
	String _package;
	String _userLanguage;
	String _appVersion;
	float _density = 1.0f;

	void setScreenSize(const Size &size) { _screenSize = size; }
	Size getScreenSize() { return _screenSize; }
	bool isTablet() { return _isTablet; }
	bool isFixed() { return _isFixed; }
	String getPackageName() { return _package; }
	float getDensity() { return _density; }
	String getUserLanguage() { return _userLanguage; }
	String getAppVersion() { return _appVersion; }
}

namespace stappler::xenolith {

int parseOptionSwitch(stappler::data::Value &ret, char c, const char *str) {
	return 1;
}

int parseOptionString(stappler::data::Value &ret, const stappler::StringView &str,
					  int argc, const char * argv[]) {
	if (str.starts_with("w=") == 0) {
		auto s = str.sub(2).readInteger().get(0);
		if (s > 0) {
			ret.setInteger(s, "width");
		}
	} else if (str.starts_with("h=") == 0) {
		auto s = str.sub(2).readInteger().get(0);
		if (s > 0) {
			ret.setInteger(s, "height");
		}
	} else if (str.starts_with("d=") == 0) {
		auto s = str.sub(2).readDouble().get(0.0);
		if (s > 0) {
			ret.setDouble(s, "density");
		}
	} else if (str.starts_with("l=") == 0) {
		ret.setString(str.sub(2), "locale");
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

SP_EXTERN_C int _spMain(argc, argv) {
	std::set_terminate(sp_android_terminate);
	std::string packageName = "org.stappler.stappler";
	stappler::data::Value val = stappler::data::readFile("app.json");
	stappler::data::Value args;
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

	args = stappler::data::parseCommandLineOptions(argc, argv, &parseOptionSwitch, &parseOptionString);

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

	platform::desktop::_screenSize = Size(args.getInteger("width"), args.getInteger("height"));
	platform::desktop::_isTablet = args.getBool("isTablet");
	platform::desktop::_package = args.getString("package");
	platform::desktop::_density = args.getDouble("density");
	platform::desktop::_isFixed = args.getBool("fixed");
	platform::desktop::_userLanguage = args.getString("locale");

    // create the application instance
    return Application::getInstance()->run();
}

}
