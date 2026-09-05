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
}
void SwitchEngine::set_responsiveness(Responsiveness r) {
  std::lock_guard<std::mutex> lock(mutex_);
  responsiveness_ = r;
  float alpha = responsiveness_alpha(r);
  for (auto &m : mappings_)
    m.ema.alpha = alpha;
}
void SwitchEngine::set_hold_time_ms(int ms) {
  std::lock_guard<std::mutex> l(mutex_);
  hold_time_ms_ = ms;
}
void SwitchEngine::set_fallback_scene(const std::string &s) {
  std::lock_guard<std::mutex> l(mutex_);
  fallback_scene_ = s;
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
    for (auto &m : mappings_) {
      if (m.audio_source == source_name) {
        m.ema.update(dbfs);
        break;
      }
    }
    target_to_switch = try_switch();
  }
  if (!target_to_switch.empty() && switch_cb_) {
    switch_cb_(target_to_switch);
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
  for (const auto &m : mappings_) {
    if (m.ema.value < m.threshold_dbfs)
      continue;
    active_speakers++;
    float w = m.ema.value + priority_bias_db(m.priority);
    if (w > best) {
      best = w;
      winner = &m;
    }
  }
  std::string target;
  if (active_speakers >= 2) {
    target = fallback_scene_;
  } else if (active_speakers == 1 && winner) {
    target = winner->scene_name;
  } else {
    target = fallback_scene_;
  }
  if (target.empty() || target == current_scene_)
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
