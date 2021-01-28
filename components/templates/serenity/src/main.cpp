
#include "ServerComponent.h"
#include "RequestHandler.h"

// Макрос для универсального определения неймспейса
// при использовании Serenity и Stellator в одном проекте
NS_SA_EXT_BEGIN(helloworld)

// Код возвращаемой страницы
static constexpr auto HELLO_WORLD_PAGE(
R"HelpString(<!DOCTYPE html>
<html>
<head><title>Hello world</title></head>
<body><h1>Hello world</h1></body>
</html>)HelpString");

// Хэндлер запроса, возвращающий страницу
class HelloWorldHandler : public RequestHandler {
public:
	virtual ~HelloWorldHandler() { }
	
	// Функция определяет доступность адреса, при неудаче возвращает 403
	virtual bool isRequestPermitted(Request &req) override;
	
	// Функция ранней обработки запроса (на стадии трансляции имени в файл)
	virtual int onTranslateName(Request &req) override;
};


// Компонент сервера, отвечающий за назначение хэндлеров
class HelloWorldComponent : public ServerComponent {
public:
	HelloWorldComponent(Server &serv, const String &name, const data::Value &dict);
	virtual ~HelloWorldComponent() { }

	// Функция инициализации сервера после выполнения всех шагов конфигурации
	virtual void onChildInit(Server &) override;
};


bool HelloWorldHandler::isRequestPermitted(Request &req) {
	return true; // разрешаем для всех
}

int HelloWorldHandler::onTranslateName(Request &req) {
	// выводим код страницы
	req << HELLO_WORLD_PAGE << "\n";
	return DONE;
}

HelloWorldComponent::HelloWorldComponent(Server &serv, const String &name, const data::Value &dict)
: ServerComponent(serv, name.empty()?"HelloWorld":name, dict) {
	
}

void HelloWorldComponent::onChildInit(Server &serv) {
	// привязываем хэндлер на все доступные адреса
	serv.addHandler("/", SA_HANDLER(HelloWorldHandler));
}

// Экспортируемая функция, которая будет нашей точкой входа в модуль
extern "C" ServerComponent * CreateHelloWorldComponent(Server &serv, const String &name, const data::Value &dict) {
	// возвращаем новый компонент
	return new HelloWorldComponent(serv, name, dict);
}

NS_SA_EXT_END(test)
