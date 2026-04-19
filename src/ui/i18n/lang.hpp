#pragma once

namespace ui::i18n {

enum class Lang : int {
    EN = 0,
    RU = 1,
    Count,
};

void set_lang(Lang lang);
Lang get_lang();

const char* t(const char* key);

}
