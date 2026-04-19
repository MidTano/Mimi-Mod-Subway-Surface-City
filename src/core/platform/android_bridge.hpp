#pragma once

#include <string>

namespace platform::android {

void set_java_vm(void* vm);
bool has_java_vm();

void request_seed_input(const std::string& current);
bool poll_seed_input(std::string& out_value);

}
