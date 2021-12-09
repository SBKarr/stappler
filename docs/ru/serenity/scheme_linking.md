Title: Схемы: связанные объекты
Priority: 0
# Схемы: связанные объекты

{{TOC}}

Хранимая в базах данных информация организована в схемы. Схемы могут быть связаны между собой различными способами

## Простая ссылка
Простые ссылки на объект другой схемы обнуляются, когда объект, на который указывает ссылка, удаляется

```cpp
	Scheme objects = Scheme("objects_with_reference");
	Scheme refs = Scheme("referenced_objects");

	objects.define({
		Field::Object("simpleRef", refs, Flags::Reference),
	});
	
	refs.define({
		...
	});
```

Если объект схемы `objects` удаляется - соответсвующий объект схемы `refs` остаёся. Если объект схемы `refs` удаляется, поле `objects.simpleRef` обнуляется, если на объект были ссылки.

## Сильная ссылка
Объекты, связанные по сильной ссылке, удаляются вместе с владельцем

```cpp
	Scheme objects = Scheme("objects_with_reference");
	Scheme refs = Scheme("referenced_objects");

	objects.define({
		Field::Object("simpleRef", refs, RemovePolicy::StrongReference),
	});
	
	refs.define({
		...
	});
```

Если объект схемы `objects` удаляется - соответсвующий объект схемы `refs` удаляется. Если объект схемы `refs` удаляется, поле `objects.simpleRef` обнуляется, если на объект были ссылки.

## Список ссылок
Ссылки из списка удаляются вместе с их объектами. Если объект, содержащий список, удаляется - объекты по ссылкам остаются.

```cpp
	Scheme objects = Scheme("objects_with_set");
	Scheme refs = Scheme("referenced_objects");

	objects.define({
		Field::Set("refSet", refs, Flags::Reference),
	});
	
	refs.define({
		...
	});
```

Если объект схемы `objects` удаляется - соответсвующие объекты схемы `refs` остаются. Если объект схемы `refs` удаляется, он удаляется и из списка `objects.refSet`.

## Список сильных ссылок
Ссылки из списка удаляются вместе с их объектами. Если объект, содержащий список, удаляется - объекты по ссылкам тоже удаляются.

```cpp
	Scheme objects = Scheme("objects_with_set");
	Scheme refs = Scheme("referenced_objects");

	objects.define({
		Field::Set("refSet", refs, RemovePolicy::StrongReference),
	});
	
	refs.define({
		...
	});
```

Если объект схемы `objects` удаляется - соответсвующие объекты схемы `refs` удаляются. Если объект схемы `refs` удаляется, он удаляется и из списка `objects.refSet`.

## Отношение один-ко-многим по ссылке
Отношение один-ко-многим даёт двусторонний доступ, вверх и вниз по иерархии вложенности, в отличии от ссылок и их списков.

```cpp
	Scheme objects = Scheme("objects");
	Scheme subobjects = Scheme("subobjects");

	objects.define({
		Field::Set("set", subobjects),
	});
	
	subobjects.define({
		Field::Object("object", objects, RemovePolicy::Null), // Это поведение по умолчанию, RemovePolicy::Null можно опустить
	});
```
Поведение аналогично простой ссылке с одной стороны и простому ссылочному списку с другой. При удалении объекта `objects` объекты `subobjects` остаются. При удалении объекта `subobjects`, он удаляется из списка.

Это поведенние по умолчанию. Оно описывается через `RemovePolicy::Null`, однако `RemovePolicy::Reference` и `Flags::Reference` дают аналогичный результат.

## Отношение один-ко-многим, каскадное удаление
```cpp
	Scheme objects = Scheme("objects");
	Scheme subobjects = Scheme("subobjects");

	objects.define({
		Field::Set("set", subobjects),
	});
	
	subobjects.define({
		Field::Object("object", objects, RemovePolicy::Cascade),
	});
```

Поведение аналогично сильному ссылочному списку со стороны `objects` и простой ссылке со стороны `subobjects`. При удалении объекта objects объекты `subobjects` ударяются. При удалении объекта `subobjects`, он удаляется из списка.

## Отношение один-ко-многим, запрет удаления
```cpp
	Scheme objects = Scheme("objects");
	Scheme subobjects = Scheme("subobjects");

	objects.define({
		Field::Set("set", subobjects),
	});
	
	subobjects.define({
		Field::Object("object", objects, RemovePolicy::Restrict),
	});
```

Если в списке у объекта `objects` есть объекты `subobjects`, его удаление завершено. При удалении объекта `subobjects`, он удаляется из списка.

## Отношение один-ко-многим, явное определение ссылки
По умолчанию, ссылки связываются автоматически по факту соотвествия между схемами спииска и объекта. Однако, в бролее слоожных схемах это может быть ошибкой. Если есть шанс, что автоматический вывод может ошибиться - ссылки нужно определить явно.

```cpp
	Scheme objects = Scheme("objects");
	Scheme subobjects = Scheme("subobjects");

	objects.define({
		Field::Set("customSet", subobjects, ForeingLink("customObject")),
	});
	
	subobjects.define({
		Field::Object("customObject", objects, ForeingLink("customSet")),
	});
```

`ForeingLink` определяет имя целевго поля в объекте чужой схемы. Сейчас позволено опредедять ссылки только между `Set` и `Object`, всё остальное - неопределённое поведение. К явным ссылкам применяются те же правила `RemovePolicy`

## Отношение один-ко-многим, ссылки на себя
Порой необходимо создать иерархию объектов одного типа. Для этого используются ссылки на себя.

```cpp
	Scheme objects = Scheme("objects");

	objects.define({
		Field::Set("selfSet", subobjects, ForeingLink("selfObject")),
		Field::Object("selfObject", subobjects, ForeingLink("selfSet")),
	});
```

К ссылкам на себя применяются те же правила `RemovePolicy`

## Другие использования
Использование правил `RemovePolicy`, а также `Flags::Reference ` не описанные в этом документе, ведут к неопределённому поведению.
