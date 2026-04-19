#include "animation.hpp"

#include <cmath>

namespace ui::fx::anim {

float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

float lerp(float a, float b, float t) { return a + (b - a) * t; }

float ease_linear(float t) { return clamp01(t); }

float ease_out_cubic(float t) {
    t = clamp01(t);
    const float u = 1.0f - t;
    return 1.0f - u * u * u;
}

float ease_in_out_cubic(float t) {
    t = clamp01(t);
    if (t < 0.5f) return 4.0f * t * t * t;
    const float u = -2.0f * t + 2.0f;
    return 1.0f - (u * u * u) * 0.5f;
}

float ease_out_back(float t) {
    t = clamp01(t);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    const float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

float ease_out_elastic(float t) {
    t = clamp01(t);
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    const float c4 = 6.2831853f / 3.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

float ease_in_quad(float t) { t = clamp01(t); return t * t; }

float spring_to(float current, float target, float dt, float speed) {
    if (dt <= 0.0f || speed <= 0.0f) return target;
    const float k = 1.0f - std::exp(-speed * dt);
    return current + (target - current) * k;
}

float advance_phase(float phase, float dt, float duration) {
    if (duration <= 0.0f) return 1.0f;
    const float next = phase + dt / duration;
    return next > 1.0f ? 1.0f : next;
}

}
