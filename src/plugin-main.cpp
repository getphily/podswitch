#include "audio-monitor.h"
#include "config.h"
#include "motion-detector.h"
#include "switch-engine.h"
#include "ui/dock-widget.h"
#include "ui/settings-dialog.h"
#include <QMainWindow>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/base.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("podswitch", "en-US")

static Config *g_config = nullptr;
static SwitchEngine *g_engine = nullptr;
static AudioMonitor *g_monitor = nullptr;
static MotionDetector *g_motion = nullptr;
static AutoCamDock *g_dock = nullptr;
static SettingsDialog *g_settings_dlg = nullptr;

static std::vector<std::string> get_audio_sources() {
  std::vector<std::string> r;
  obs_enum_sources(
      [](void *p, obs_source_t *s) -> bool {
        auto *v = static_cast<std::vector<std::string> *>(p);
        if (obs_source_get_output_flags(s) & OBS_SOURCE_AUDIO)
          v->push_back(obs_source_get_name(s));
        return true;
      },
      &r);
  return r;
}
static std::vector<std::string> get_video_sources() {
  std::vector<std::string> r;
  obs_enum_sources(
      [](void *p, obs_source_t *s) -> bool {
        auto *v = static_cast<std::vector<std::string> *>(p);
        if (obs_source_get_output_flags(s) & OBS_SOURCE_VIDEO)
          v->push_back(obs_source_get_name(s));
        return true;
      },
      &r);
  return r;
}
static std::vector<std::string> get_scene_names() {
  std::vector<std::string> r;
  char **n = obs_frontend_get_scene_names();
  if (n) {
    for (char **p = n; *p; ++p)
      r.push_back(*p);
    bfree(n);
  }
  return r;
}
static void apply_config() {
  g_engine->set_mappings(g_config->get_mappings());
  g_engine->set_responsiveness(g_config->get_responsiveness());
  g_engine->set_hold_time_ms(g_config->get_hold_time_ms());
  g_monitor->clear();
  if (g_motion) g_motion->clear();
  for (auto &m : g_config->get_mappings()) {
    g_monitor->add_source(m.audio_source);
    if (g_motion && !m.video_source.empty()) g_motion->add_source(m.video_source);
  }
  if (g_dock) {
    std::vector<std::pair<std::string, std::string>> pairs;
    for (auto &m : g_config->get_mappings())
      pairs.emplace_back(m.audio_source, m.scene_name);
    g_dock->refresh_mappings(pairs);
  }
}
static void do_switch(const std::string &scene_name) {
  auto *name = new std::string(scene_name);
  obs_queue_task(
      OBS_TASK_UI,
      [](void *param) {
        auto *name = static_cast<std::string *>(param);
        if (g_config && g_config->get_transition_fade()) {
          obs_source_t *ft = obs_get_source_by_name("Fade");
          if (ft) {
            obs_frontend_set_current_transition(ft);
            obs_frontend_set_transition_duration(
                g_config->get_fade_duration_ms());
            obs_source_release(ft);
          }
        }
        obs_source_t *scene = obs_get_source_by_name(name->c_str());
        if (scene) {
          obs_frontend_set_current_scene(scene);
          obs_source_release(scene);
        }
        delete name;
      },
      name, false);
}
static void on_frontend_event(enum obs_frontend_event event, void *) {
  if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING ||
      event == OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED) {
    apply_config();
    if (g_settings_dlg)
      g_settings_dlg->populate_sources(get_audio_sources(), get_video_sources(), get_scene_names());
  } else if (event == OBS_FRONTEND_EVENT_SCENE_CHANGED) {
    obs_source_t *scene = obs_frontend_get_current_scene();
    if (scene) {
      if (g_engine) g_engine->sync_current_scene(obs_source_get_name(scene));
      obs_source_release(scene);
    }
  }
}
extern void podswitch_register_motion_filter();

bool obs_module_load() {
  blog(LOG_INFO, "[podswitch] Loading PodSwitch v1.1.0");
  podswitch_register_motion_filter();
  g_config = new Config();
  g_engine = new SwitchEngine();
  g_monitor = new AudioMonitor();
  g_monitor->set_callback(
      [](const std::string &s, float d) { g_engine->on_audio_level(s, d); });
  g_motion = new MotionDetector();
  g_motion->set_callback([](const std::string &s, float e) {
    if (g_engine) g_engine->update_motion_energy(s, e);
  });
  g_engine->set_switch_callback(do_switch);
  apply_config();
  obs_frontend_add_event_callback(on_frontend_event, nullptr);
  g_dock = new AutoCamDock(g_engine);
  obs_frontend_add_dock_by_id("podswitch-dock", "PodSwitch", g_dock);
  QObject::connect(g_dock, &AutoCamDock::responsiveness_changed, [](int r) {
    if (g_config) {
      g_config->set_responsiveness((Responsiveness)r);
      g_config->save();
    }
  });
  g_settings_dlg = new SettingsDialog(g_config);
  QObject::connect(g_settings_dlg, &SettingsDialog::settings_applied,
                   [](const Config &) {
                     apply_config();
                     if (g_dock && g_config) {
                       g_dock->set_responsiveness(g_config->get_responsiveness());
                     }
                   });
  QObject::connect(g_dock, &AutoCamDock::open_settings_requested, []() {
    g_settings_dlg->populate_sources(get_audio_sources(), get_video_sources(), get_scene_names());
    g_settings_dlg->show();
    g_settings_dlg->raise();
    g_settings_dlg->activateWindow();
  });
  g_dock->set_responsiveness(g_config->get_responsiveness());
  blog(LOG_INFO, "[podswitch] Loaded OK");
  return true;
}
void obs_module_unload() {
  obs_frontend_remove_event_callback(on_frontend_event, nullptr);
  if (g_engine) g_engine->set_enabled(false);
  if (g_monitor) g_monitor->clear();
  if (g_motion) g_motion->clear();
  delete g_motion;
  delete g_monitor;
  delete g_engine;
  delete g_config;
  g_dock = nullptr;
  g_settings_dlg = nullptr;
  g_motion = nullptr;
  g_monitor = nullptr;
  g_engine = nullptr;
  g_config = nullptr;
}
const char *obs_module_name() { return "PodSwitch"; }
const char *obs_module_description() { return "Podcast auto camera switcher."; }
