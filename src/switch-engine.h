#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <functional>
#include <cfloat>
#include "utils.h"

enum class Priority { Low = 0, Medium = 1, High = 2 };
inline float priority_bias_db(Priority p) {
    if (p == Priority::Low)  return -6.0f;
    if (p == Priority::High) return +6.0f;
    return 0.0f;
}

enum class Responsiveness { Relaxed = 0, Neutral = 1, Fast = 2 };
inline float responsiveness_alpha(Responsiveness r) {
    if (r == Responsiveness::Relaxed) return 0.05f;
    if (r == Responsiveness::Fast)    return 0.40f;
    return 0.15f;
}

enum class MotionInfluence { Off = 0, Moderate = 1, High = 2 };
inline float motion_influence_weight(MotionInfluence m) {
    if (m == MotionInfluence::Moderate) return 0.5f;
    if (m == MotionInfluence::High) return 1.5f;
    return 0.0f;
}

struct CamMapping {
    std::string audio_source;
    std::string video_source;
    std::string scene_name;
    Priority    priority        = Priority::Medium;
    float       threshold_dbfs  = -40.0f;
    EMA         ema;
    float       motion_energy   = 0.0f;
};

struct SourceLevels {
    std::string source_name;
    float audio_dbfs;
    float motion_energy;
};

using SwitchCallback = std::function<void(const std::string &scene_name)>;

class SwitchEngine {
public:
    SwitchEngine();
    void set_mappings(std::vector<CamMapping> mappings);
    void set_responsiveness(Responsiveness r);
    void set_motion_influence(MotionInfluence m);
    void set_switch_callback(SwitchCallback cb);
    void on_audio_level(const std::string &source_name, float dbfs);
    void update_motion_energy(const std::string &source_name, float energy);
    void set_enabled(bool enabled);
    bool is_enabled() const { return enabled_; }
    std::vector<SourceLevels> get_levels() const;
    std::string get_current_scene() const;
    void sync_current_scene(const std::string &scene);
private:
    std::string try_switch();
    mutable std::mutex mutex_;
    std::vector<CamMapping> mappings_;
    std::unordered_map<std::string, size_t> audio_to_mapping_;
    std::unordered_map<std::string, size_t> video_to_mapping_;
    Responsiveness responsiveness_ = Responsiveness::Neutral;
    MotionInfluence motion_influence_ = MotionInfluence::Moderate;
    const int hold_time_ms_ = 800;
    SwitchCallback switch_cb_;
    std::atomic<bool> enabled_{false};
    std::string current_scene_;
    std::chrono::steady_clock::time_point last_switch_time_;
};
