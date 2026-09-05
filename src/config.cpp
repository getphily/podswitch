#include "config.h"
#include <obs-data.h>
#include <obs-module.h>
#include <util/base.h>

static const char *resp_str(Responsiveness r) {
  if (r == Responsiveness::Relaxed)
    return "relaxed";
  if (r == Responsiveness::Fast)
    return "fast";
  return "neutral";
}
static Responsiveness str_resp(const char *s) {
  if (s && std::string(s) == "relaxed")
    return Responsiveness::Relaxed;
  if (s && std::string(s) == "fast")
    return Responsiveness::Fast;
  return Responsiveness::Neutral;
}
static const char *motion_inf_str(MotionInfluence m) {
  if (m == MotionInfluence::Off)
    return "off";
  if (m == MotionInfluence::High)
    return "high";
  return "moderate";
}
static MotionInfluence str_motion_inf(const char *s) {
  if (s && std::string(s) == "off")
    return MotionInfluence::Off;
  if (s && std::string(s) == "high")
    return MotionInfluence::High;
  return MotionInfluence::Moderate;
}
static const char *reaction_cutaways_str(ReactionCutaways r) {
  if (r == ReactionCutaways::Occasional)
    return "occasional";
  if (r == ReactionCutaways::Often)
    return "often";
  return "never";
}
static ReactionCutaways str_reaction_cutaways(const char *s) {
  if (s && std::string(s) == "occasional")
    return ReactionCutaways::Occasional;
  if (s && std::string(s) == "often")
    return ReactionCutaways::Often;
  return ReactionCutaways::Never;
}
static const char *prio_str(Priority p) {
  if (p == Priority::Low)
    return "low";
  if (p == Priority::High)
    return "high";
  return "medium";
}
static Priority str_prio(const char *s) {
  if (s && std::string(s) == "low")
    return Priority::Low;
  if (s && std::string(s) == "high")
    return Priority::High;
  return Priority::Medium;
}
static const char *threshold_str(ThresholdLevel t) {
  if (t == ThresholdLevel::Quiet)
    return "quiet";
  if (t == ThresholdLevel::Loud)
    return "loud";
  return "normal";
}
static ThresholdLevel str_threshold(const char *s) {
  if (s && std::string(s) == "quiet")
    return ThresholdLevel::Quiet;
  if (s && std::string(s) == "loud")
    return ThresholdLevel::Loud;
  return ThresholdLevel::Normal;
}

Config::Config() { load(); }
Config::~Config() {}

std::string Config::get_config_path() const {
  char *path = obs_module_config_path("settings.json");
  std::string p = path ? path : "podswitch-settings.json";
  if (path)
    bfree(path);
  return p;
}
void Config::load() {
  auto path = get_config_path();
  obs_data_t *d = obs_data_create_from_json_file(path.c_str());
  if (!d) {
    blog(LOG_INFO, "[switchy] No saved config");
    return;
  }
  responsiveness_ = str_resp(obs_data_get_string(d, "responsiveness"));
  motion_influence_ = str_motion_inf(obs_data_get_string(d, "motion_influence"));
  reaction_cutaways_ = str_reaction_cutaways(obs_data_get_string(d, "reaction_cutaways"));
  gen_format_ = (int)obs_data_get_int(d, "gen_format");
  obs_data_array_t *arr = obs_data_get_array(d, "mappings");
  size_t n = arr ? obs_data_array_count(arr) : 0;
  mappings_.clear();
  for (size_t i = 0; i < n; ++i) {
    obs_data_t *item = obs_data_array_item(arr, i);
    CamMapping m;
    m.audio_source = obs_data_get_string(item, "audio_source");
    m.video_source = obs_data_get_string(item, "video_source");
    m.scene_name = obs_data_get_string(item, "scene_name");
    m.priority = str_prio(obs_data_get_string(item, "priority"));
    
    // Backwards compatibility for old config which stored double
    if (obs_data_has_user_value(item, "threshold")) {
      m.threshold = str_threshold(obs_data_get_string(item, "threshold"));
    } else if (obs_data_has_user_value(item, "threshold_dbfs")) {
      double old_val = obs_data_get_double(item, "threshold_dbfs");
      if (old_val <= -45.0) m.threshold = ThresholdLevel::Quiet;
      else if (old_val >= -25.0) m.threshold = ThresholdLevel::Loud;
      else m.threshold = ThresholdLevel::Normal;
    } else {
      m.threshold = ThresholdLevel::Normal;
    }
    
    obs_data_release(item);
    mappings_.push_back(std::move(m));
  }
  if (arr)
    obs_data_array_release(arr);
  obs_data_release(d);
  blog(LOG_INFO, "[podswitch] Loaded %zu mappings", mappings_.size());
}
void Config::save() const {
  obs_data_t *d = obs_data_create();
  obs_data_set_string(d, "responsiveness", resp_str(responsiveness_));
  obs_data_set_string(d, "motion_influence", motion_inf_str(motion_influence_));
  obs_data_set_string(d, "reaction_cutaways", reaction_cutaways_str(reaction_cutaways_));
  obs_data_set_int(d, "gen_format", gen_format_);
  obs_data_array_t *arr = obs_data_array_create();
  for (const auto &m : mappings_) {
    obs_data_t *item = obs_data_create();
    obs_data_set_string(item, "audio_source", m.audio_source.c_str());
    obs_data_set_string(item, "video_source", m.video_source.c_str());
    obs_data_set_string(item, "scene_name", m.scene_name.c_str());
    obs_data_set_string(item, "priority", prio_str(m.priority));
    obs_data_set_string(item, "threshold", threshold_str(m.threshold));
    obs_data_array_push_back(arr, item);
    obs_data_release(item);
  }
  obs_data_set_array(d, "mappings", arr);
  obs_data_array_release(arr);
  obs_data_save_json(d, get_config_path().c_str());
  obs_data_release(d);
  blog(LOG_INFO, "[podswitch] Config saved");
}
