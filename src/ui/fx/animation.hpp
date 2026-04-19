#pragma once

namespace ui::fx::anim {

float clamp01(float v);
float lerp(float a, float b, float t);
float ease_linear(float t);
float ease_out_cubic(float t);
float ease_in_out_cubic(float t);
float ease_out_back(float t);
float ease_out_elastic(float t);
float ease_in_quad(float t);

float spring_to(float current, float target, float dt, float speed);

float advance_phase(float phase, float dt, float duration);

}
