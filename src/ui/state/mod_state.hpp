#pragma once

#include <map>
#include <string>
#include <string_view>
#include <variant>

namespace ui::state {

using Value = std::variant<bool, int, float, std::string>;

using Store = std::map<std::string, Value, std::less<>>;

void set_bool(std::string_view key, bool v);
void set_int(std::string_view key, int v);
void set_float(std::string_view key, float v);
void set_string(std::string_view key, std::string_view v);

bool  get_bool(std::string_view key, bool fallback = false);
int   get_int(std::string_view key, int fallback = 0);
float get_float(std::string_view key, float fallback = 0.0f);
std::string get_string(std::string_view key, std::string_view fallback = {});

bool has(std::string_view key);
void erase(std::string_view key);
void clear_all();

const Store& all();

void mark_dirty();
bool is_dirty();
void clear_dirty();

}
