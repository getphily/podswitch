#pragma once
#include <cmath>
#include <cstdint>

inline float linear_to_dbfs(float linear) {
    if (linear <= 0.0f) return -96.0f;
    return 20.0f * std::log10(linear);
}

inline float compute_rms(const float *data, uint32_t frames, uint32_t channels) {
    if (!data) return 0.0f;
    double sum = 0.0;
    uint32_t total = frames * channels;
    if (total == 0) return 0.0f;
    for (uint32_t i = 0; i < total; ++i) {
        double v = (double)data[i];
        sum += v * v;
    }
    return (float)std::sqrt(sum / (double)total);
}

struct EMA {
    float value = -96.0f;
    float alpha = 0.15f;
    void update(float newVal) {
        if (std::isnan(newVal) || std::isinf(newVal)) return;
        value = alpha * newVal + (1.0f - alpha) * value;
    }
};
