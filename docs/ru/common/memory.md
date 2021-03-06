Title: Управление памятью
Priority: 0
# Управление памятью
{{TOC}}
В Stappler используются две схемы управления памятью: стандартная и mempool. Различные компоненты могут использовать разную основную схему памяти, от этого зависит способ имплементации (но не API) базовых контейнеров и типов, таких как:
* String
* Vector
* Set
* Map
* Function
* Mutex

## Режим памяти mem_pool

Этот режим используется по умолчанию в компонентах Stellator и Serenity. Кроме того, пулы памяти используют различные вычислительные функции в других компонентах, такие как функции тесселяции в Layout, вычисление расстояния Эдельштейна и прочие.

**Этот режим памяти серьёзно отличается от классического подхода к управлению памятью**

## Режим памяти mem_std

Это стандартный режим памяти, используемый в стандартной библиотеке, поставляемой в вашей ОС. Он не требует особых навыков обращения и подходит для большинства случаев, однако в ряде задач не даёт достаточной производительности.
