#include "switch-engine.h"
#include <algorithm>
#include <util/base.h>

SwitchEngine::SwitchEngine() {
  last_switch_time_ =
      std::chrono::steady_clock::now() - std::chrono::milliseconds(99999);
}
void SwitchEngine::set_mappings(std::vector<CamMapping> mappings) {
  std::lock_guard<std::mutex> lock(mutex_);
  float alpha = responsiveness_alpha(responsiveness_);
  for (auto &m : mappings)
    m.ema.alpha = alpha;
  mappings_ = std::move(mappings);
  audio_to_mapping_.clear();
  video_to_mapping_.clear();
  for (size_t i = 0; i < mappings_.size(); ++i) {
      if (!mappings_[i].audio_source.empty())
          audio_to_mapping_[mappings_[i].audio_source] = i;
      if (!mappings_[i].video_source.empty())
          video_to_mapping_[mappings_[i].video_source] = i;
  }
}
void SwitchEngine::set_responsiveness(Responsiveness r) {
  std::lock_guard<std::mutex> lock(mutex_);
  responsiveness_ = r;
  float alpha = responsiveness_alpha(r);
  for (auto &m : mappings_)
    m.ema.alpha = alpha;
}
void SwitchEngine::set_motion_influence(MotionInfluence m) {
  std::lock_guard<std::mutex> lock(mutex_);
  motion_influence_ = m;
}
void SwitchEngine::set_switch_callback(SwitchCallback cb) {
  std::lock_guard<std::mutex> l(mutex_);
  switch_cb_ = std::move(cb);
}
void SwitchEngine::set_enabled(bool enabled) {
  std::lock_guard<std::mutex> lock(mutex_);
  enabled_ = enabled;
  blog(LOG_INFO, "[switchy] %s", enabled ? "ENABLED" : "DISABLED");
}
void SwitchEngine::on_audio_level(const std::string &source_name, float dbfs) {
  if (!enabled_)
    return;
  std::string target_to_switch;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = audio_to_mapping_.find(source_name);
    if (it != audio_to_mapping_.end()) {
        mappings_[it->second].ema.update(dbfs);
    }
    target_to_switch = try_switch();
  }
  if (!target_to_switch.empty() && switch_cb_) {
    switch_cb_(target_to_switch);
  }
}
void SwitchEngine::update_motion_energy(const std::string &source_name, float energy) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = video_to_mapping_.find(source_name);
  if (it != video_to_mapping_.end()) {
      mappings_[it->second].motion_energy = energy;
  }
}
void SwitchEngine::sync_current_scene(const std::string &scene) {
  std::lock_guard<std::mutex> lock(mutex_);
  current_scene_ = scene;
}
std::string SwitchEngine::try_switch() {
  float best = -FLT_MAX;
  const CamMapping *winner = nullptr;
  int active_speakers = 0;
  float motion_weight = motion_influence_weight(motion_influence_);
  for (const auto &m : mappings_) {
    float score = m.ema.value + priority_bias_db(m.priority) +
                  motion_weight * (m.motion_energy / 10.0f);
    if (m.ema.value >= m.threshold_dbfs)
      active_speakers++;
    if (score > best) {
      best = score;
      winner = &m;
    }
  }
  std::string target;
  if (active_speakers >= 2) {
    target = "";
  } else if (winner) {
    target = winner->scene_name;
  } else {
    target = "";
  }

  // If the engine gets confused (crosstalk or silence),
  // we do not want it to instantly jump as soon as someone speaks. We enforce a timeout.
  if (target.empty() || target == "— None —") {
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - last_switch_time_)
                         .count();
      if (elapsed >= hold_time_ms_) {
          last_switch_time_ = now;
      }
      return "";
  }

  if (target == current_scene_)
    return "";
    
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     now - last_switch_time_)
                     .count();
  if (elapsed < hold_time_ms_)
    return "";
    
  current_scene_ = target;
  last_switch_time_ = now;
  blog(LOG_DEBUG, "[switchy] -> '%s'", target.c_str());
  return target;
}
std::vector<SourceLevels> SwitchEngine::get_levels() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<SourceLevels> result;
  for (const auto &m : mappings_)
    result.push_back({m.audio_source, m.ema.value, m.motion_energy});
  return result;
}
std::string SwitchEngine::get_current_scene() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_scene_;
}
