Title: Serenity
Priority: 4
# Компонент Serenity

{{TOC}}

# Кодирование строки запроса

Serenity поддерживает стандартное URL-кодирование с дополнениями:

```
https://example.com/path?name=value&array[]=arrayValue1&array[]=arrayValue2&dict[dictKey]=dictValue
```


Более удобно и компактно использовать [собственный код Serenity](encoding):

```
https://example.com/path?(name:value;array:value1,value2;dict(key:value))
```

# База данных

Serenity использует интерфейс базы данных от Stellator.

# Функции Web-сервера

## Алгоритм сжатия Brotli

Serenity не поддерживает традиционное сжатие на основе gzip и deflate, но поддерживает алгоритм [Brotli](https://ru.wikipedia.org/wiki/Brotli).
Для использования нужно передать корректный заголовок `Accept-Encoding: br`.

## Кодирование CBOR

Serenity поддерживает кодирование данных JSON `application/json` и `application/x-www-form-urlencoded` и `multipart/form-data`. Для более эффективного кодирования можно использовать формат [CBOR](https://cbor.io/) `application/cbor`. Кодирование в CBOR эффективнее JSON около 1.5-2 раз. Для использования нужно передать корректный заголовок `Accept`, содержащий тип `application/cbor`. Вне зависимости от указанных приоритетов, Serenity будет использовать CBOR, если он есть в списке допустимых.

## Обход HTTPS для ACME

Для использования с ACME-клиентами (в т.ч. certbot от Let's Encrypt) Serenity позволяет доступ к адресам за `http://example.com/.well-known/acme-challenge/` без принудительного HTTPS. Таким образом, можно использовать certbot с провайдером webroot для получения новых сертификтов.

## Конфигурация

`SerenitySourceRoot <Path>` - путь для загрузки клиентских модулей Serenity.

`SerenitySource [<Name>:]<File>:<Func> param=value ..` - параметры для загрузки клиентского модуля где `Name` - опциональное имя модуля (иначе, будет использовано установленное по умолчанию), `File` - имя файла библиотеки, в которой искать загрузчик, `Func` - имя функции-загрузчика в файле, функция должна быть доступна в формате линковки C FFI (`extern C`). Список именованых параметров будет передан в функцию загрузки в виде `data::Value`.

`SerenityForceHttps 1` - требование принудительного HTTPS для хоста. Устанавливается для HTTP хостов. Для работы ACME необходимо указать параметр `DocumentRoot`, все остальные параметры хоста можно игнорировать.

`SerenityProtected <URL1>, <URL2>, ...` - список префиксов URL, доступ к которым будет заблокирован.
