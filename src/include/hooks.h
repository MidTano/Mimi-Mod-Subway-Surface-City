#pragma once

namespace Hooks {

class HookManager {
public:
    static HookManager& getInstance();

    void initialize();
    void shutdown();

private:
    HookManager();
    ~HookManager();

    HookManager(const HookManager&) = delete;
    HookManager& operator=(const HookManager&) = delete;
};

}
