#pragma once
#include <string>
#include <vector>
#include "switch-engine.h"

class Config {
public:
    Config(); ~Config();
    std::vector<CamMapping> get_mappings() const { return mappings_; }
    void set_mappings(const std::vector<CamMapping> &m) { mappings_ = m; }
    Responsiveness get_responsiveness() const { return responsiveness_; }
    void set_responsiveness(Responsiveness r) { responsiveness_ = r; }
    MotionInfluence get_motion_influence() const { return motion_influence_; }
    void set_motion_influence(MotionInfluence m) { motion_influence_ = m; }
    int get_hold_time_ms() const { return hold_time_ms_; }
    void set_hold_time_ms(int ms) { hold_time_ms_ = ms; }
    std::string get_fallback_scene() const { return fallback_scene_; }
    void set_fallback_scene(const std::string &s) { fallback_scene_ = s; }
    bool get_transition_fade() const { return transition_fade_; }
    void set_transition_fade(bool f) { transition_fade_ = f; }
    int get_fade_duration_ms() const { return fade_duration_ms_; }
    void set_fade_duration_ms(int ms) { fade_duration_ms_ = ms; }

    int get_gen_format() const { return gen_format_; }
    void set_gen_format(int f) { gen_format_ = f; }

    void load(); void save() const;
private:
    std::vector<CamMapping> mappings_;
    Responsiveness responsiveness_ = Responsiveness::Neutral;
    MotionInfluence motion_influence_ = MotionInfluence::Moderate;
    int hold_time_ms_ = 800;
    std::string fallback_scene_;
    bool transition_fade_ = false;
    int fade_duration_ms_ = 300;
    
    int gen_format_ = 0;

    std::string get_config_path() const;
};
