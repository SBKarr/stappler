#include "SPCommon.h"
#include "SPData.h" // для data::Value и data::parseCommandLineOptions

// Многострочный строковый литерал, привет, Modern C++
static constexpr auto HELP_STRING(
R"HelpString(test - my first Stappler app, writes 'Hello World'
Options are one of:
    -v (--verbose) - write information about dirs and parsed arguments
    -h (--help) - write this message)HelpString");

// простейщий способ работы - использовать этот неймспейс
// Другие описаны в разделе об управлении памятью
namespace stappler {

// Читаем единичные опции, переданные через -<v>
static int parseOptionSwitch(data::Value &ret, char c, const char *str) {
	if (c == 'h') {
		ret.setBool(true, "help");
	} else if (c == 'v') {
		ret.setBool(true, "verbose");
	}
	return 1; // возвращаем число прочитанных символов
}

// Читаем строки, переданные через --<value>
int parseOptionString(data::Value &ret, const StringView &str, int argc, const char * argv[]) {
	if (str == "help") {
		ret.setBool(true, "help");
	} else if (str == "verbose") {
		ret.setBool(true, "verbose");
	}
	return 1; // возвращаем число прочитанных слов
}

// Функция main, макрос _spMain (argc, argv) обеспечивает платформонезависимость
SP_EXTERN_C // Требуем для main линковки в стиле "C"
int _spMain(argc, argv) {
	data::Value opts = data::parseCommandLineOptions(argc, argv,
			&parseOptionSwitch, &parseOptionString); // разбираем опции командной строки
	
	// Если передана -h или --help - показываем строку и выходим
	if (opts.getBool("help")) {
		std::cout << HELP_STRING << "\n";
		return 0;
	}

	// Если передана -v или --verbose - показываем дополнительную информацию
	if (opts.getBool("verbose")) {
		std::cout << " Current work dir: " << stappler::filesystem::currentDir() << "\n";
		std::cout << " Documents dir: " << stappler::filesystem::documentsPath() << "\n";
		std::cout << " Cache dir: " << stappler::filesystem::cachesPath() << "\n";
		std::cout << " Writable dir: " << stappler::filesystem::writablePath() << "\n";
		std::cout << " Options: " << stappler::data::EncodeFormat::Pretty << opts << "\n";
	}

	// Пишем требуемую строку и выходим
	std::cout << "Hello world\n";

	return 0;
}

} // namespace stappler
