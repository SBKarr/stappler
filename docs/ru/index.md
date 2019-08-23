
# Документация

{{TOC}}

## Компоненты

[Common][] - общий компонент фреймворка, который реализует управление памятью, общие операции со строками, бинарными данными, нетипизированными данными

[Document][] - работа с документами формата [MultiMarkdown][] и [EPub][], в том числе общие функции, функции для графического вывода и компоненты для графических приложений  

[Layout][] - низкоуровневые функции для работы с графическим движком, в том числе для графического форматироввания реьд и смежных форматов

[Material][] - реализация графических компонентов Material Design

[Serenity][] - серверный компонент (модуль для [HTTPD][]), реализующий фильтрацию и генерацию контента

[SPug][] - реализация формата шаблонизатора [Pug][]

[Stappler][] - графический движок

[Stellator][] - вспомогательный компонент для работы с базой данных

## Система сборки

Система сборки основана на [Make][] и упрощает создание приложений на базе фреймворка

[Common]: common
[Document]: document
[Layout]: layout
[Material]: material
[SPug]: spug
[Serenity]: serenity
[Stappler]: stappler
[Stellator]: stellator

[MultiMarkdown]: https://rawgit.com/fletcher/MultiMarkdown-6-Syntax-Guide/master/index.html
[EPub]: https://ru.wikipedia.org/wiki/Electronic_Publication
[Pug]: https://pugjs.org/api/getting-started.html
[HTTPD]: https://httpd.apache.org/
[Make]: https://www.gnu.org/software/make/
