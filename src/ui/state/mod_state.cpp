#include "mod_state.hpp"

namespace ui::state {

namespace {
Store g_store;
bool g_dirty = false;

template <class V>
void set_impl(std::string_view k, V&& v) {
    auto it = g_store.find(k);
    if (it == g_store.end()) g_store.emplace(std::string(k), std::forward<V>(v));
    else it->second = std::forward<V>(v);
    g_dirty = true;
}
}

void set_bool(std::string_view k, bool v)                 { set_impl(k, v); }
void set_int(std::string_view k, int v)                   { set_impl(k, v); }
void set_float(std::string_view k, float v)               { set_impl(k, v); }
void set_string(std::string_view k, std::string_view v)   { set_impl(k, std::string(v)); }

bool get_bool(std::string_view k, bool fb) {
    auto it = g_store.find(k);
    if (it == g_store.end()) return fb;
    if (auto* p = std::get_if<bool>(&it->second)) return *p;
    return fb;
}
int get_int(std::string_view k, int fb) {
    auto it = g_store.find(k);
    if (it == g_store.end()) return fb;
    if (auto* p = std::get_if<int>(&it->second)) return *p;
    if (auto* p = std::get_if<float>(&it->second)) return static_cast<int>(*p);
    return fb;
}
float get_float(std::string_view k, float fb) {
    auto it = g_store.find(k);
    if (it == g_store.end()) return fb;
    if (auto* p = std::get_if<float>(&it->second)) return *p;
    if (auto* p = std::get_if<int>(&it->second)) return static_cast<float>(*p);
    return fb;
}
std::string get_string(std::string_view k, std::string_view fb) {
    auto it = g_store.find(k);
    if (it == g_store.end()) return std::string(fb);
    if (auto* p = std::get_if<std::string>(&it->second)) return *p;
    return std::string(fb);
}

bool has(std::string_view k)   { return g_store.find(k) != g_store.end(); }
void erase(std::string_view k) {
    auto it = g_store.find(k);
    if (it != g_store.end()) { g_store.erase(it); g_dirty = true; }
}
void clear_all() { g_store.clear(); g_dirty = true; }

const Store& all() { return g_store; }

void mark_dirty()  { g_dirty = true; }
bool is_dirty()    { return g_dirty; }
void clear_dirty() { g_dirty = false; }

}
