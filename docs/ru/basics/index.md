Title: Основы
Priority: 11
# Основы

{{TOC}}

## Начало
Исходный код и большинство необходимых для сборки библиотек доступны на github:

```sh
git pull https://github.com/SBKarr/stappler
cd stappler
git submodule update --init
```

Проект максимально самодостаточен, но для отдельных компонентов могут быть нужны дополнительные библиотеки, об этом в соответствующих разделах. Основная платформа проекта - Linux, работоспособность на Mac и Windows ограничена, полная поддержка не планируется. Однако, настольная часть проекта поддерживает контейнеризацию в [Docker](docker).

Для сборки необходимы **make**, **gcc** и **ld.gold**, доступные в стандартных репозиториях. Для Mac используются компоненты в составе **XCode**. Для Windows используется **MSys** и линкер **lld** вместо ld.gold. Для Android необходимо установить **[Android NDK](https://developer.android.com/ndk)**.

Проект использует С++17, компилятор и стандартная библиотека должны поддерживать как минимум эту версию диалекта.

Проект не требует предварительной сборки, если нет необходимости использовать компоненты как статические или динамические библиотеки. Если такая необходимость есть, нужно запустить
```sh
make all
```

Запуск Serenity описан в [отдельном разделе](serenity).

## Hello world
Сперва создадим рутовую директорию проекта:
```sh
cd ..
mkdir MyNewProject
cd MyNewProject
touch Makefile
touch main.cpp
```

Для первого приложения необходимо создать Makefile и файл исходного кода C++, содержащий функцию main.

**Makefile**:
```make
STAPPLER_ROOT = ../stappler #путь к рутовой директории фреймворка

LOCAL_ROOT = . # корневая директория проекта, обычно устанавливается в текущую ( . )
LOCAL_OUTDIR := build # директория файлов сборки
LOCAL_EXECUTABLE := test # название исполняемого файла, который будет собран

LOCAL_TOOLKIT := common # используемый набор инструментов. Common - минимальный набор с минимальными требованиями

LOCAL_SRCS_DIRS := # список директорий для компиляции
LOCAL_SRCS_OBJS := # список отдельных файлов для компиляции

LOCAL_INCLUDES_DIRS := # список директорий для включения (передаётся в -I компилятора)
LOCAL_INCLUDES_OBJS := # список отдельных элементов для включения (передаётся в -I компилятора)

LOCAL_MAIN := main.cpp # файл, содержащий функцию main

LOCAL_LIBS = # список дополнительных статических и динамических библиотек для линковки

include $(STAPPLER_ROOT)/make/universal.mk # инициализация универсального сборщика
```

**main.cpp**:
```cpp
#include "SPCommon.h"

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
```

После чего собираем проект для текущей системы:

```sh
make host-install # собирает и копирует исполняемый файл из сборочной директории

#  Собранный файл можно найти по адресу build/host/test
```
