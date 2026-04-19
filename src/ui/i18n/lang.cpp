#include "lang.hpp"
#include "lang_data.hpp"

#include <atomic>
#include <cstring>

namespace ui::i18n {

namespace {
std::atomic<int> g_lang{static_cast<int>(Lang::EN)};
}

void set_lang(Lang lang) {
    int v = static_cast<int>(lang);
    if (v < 0) v = 0;
    if (v >= static_cast<int>(Lang::Count)) v = 0;
    g_lang.store(v);
}

Lang get_lang() {
    return static_cast<Lang>(g_lang.load());
}

const char* t(const char* key) {
    if (key == nullptr || *key == '\0') return "";
    const int lang = g_lang.load();
    for (int i = 0; i < data::kEntriesCount; ++i) {
        const auto& e = data::kEntries[i];
        if (std::strcmp(e.key, key) == 0) {
            switch (static_cast<Lang>(lang)) {
                case Lang::RU: return e.ru != nullptr ? e.ru : e.en;
                case Lang::EN:
                default:       return e.en;
            }
        }
    }
    return key;
}

}
