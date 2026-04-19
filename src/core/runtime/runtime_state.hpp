#pragma once

#include <atomic>

namespace runtime {

struct State {
    std::atomic<bool> menu_visible{false};
    std::atomic<bool> fqa_overlay_active{false};
    std::atomic<float> user_time_scale{1.0f};
    std::atomic<float> applied_time_scale{1.0f};
    std::atomic<float> dim_amount{0.0f};
};

State& state();

}
