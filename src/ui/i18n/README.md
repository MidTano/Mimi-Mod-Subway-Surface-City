# Локализация

## Как добавить новый язык

1. В `lang.hpp` добавить значение в enum `Lang`:
```cpp
enum class Lang : int {
    EN = 0,
    RU = 1,
    DE = 2,
    Count,
};
```

2. В `lang_data.hpp` в struct `Entry` добавить поле `de`:
```cpp
struct Entry {
    const char* key;
    const char* en;
    const char* ru;
    const char* de;
};
```

3. Для каждой записи в массиве `kEntries` добавить немецкий перевод четвёртым аргументом.

4. В `lang.cpp` в функции `t()` обработать новый кейс:
```cpp
case Lang::DE: return e.de != nullptr ? e.de : e.en;
```

5. В UI (`sections/sections.cpp`) добавить пункт в массив `lang_opts`:
```cpp
static const char* lang_opts[] = { "EN", "RU", "DE" };
```
И в call `card_segmented` увеличить размер до `3`.

## Правила

- Русский текст нужно писать в UTF-8 escape-форме (`\xD0\xA1...`), либо редактировать файл как UTF-8 (без BOM).
- Если перевод отсутствует для языка — используется английский fallback.
- Для новых ключей: добавить Entry с `key` + переводом во всех языках.
- Ключ должен совпадать со строкой, которая передаётся в `ui::i18n::t("KEY")`.
