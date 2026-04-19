#pragma once

namespace Features {

class FeatureManager {
public:
    static FeatureManager& getInstance();

    void initialize();
    void update();
    void shutdown();

private:
    FeatureManager();
    ~FeatureManager();

    FeatureManager(const FeatureManager&) = delete;
    FeatureManager& operator=(const FeatureManager&) = delete;
};

}
