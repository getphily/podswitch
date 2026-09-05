#include "audio-monitor.h"
#include "utils.h"
#include <obs-module.h>
#include <util/base.h>

AudioMonitor::AudioMonitor() {}
AudioMonitor::~AudioMonitor() { clear(); }

void AudioMonitor::set_callback(AudioLevelCallback cb) {
  std::lock_guard<std::mutex> lock(mutex_);
  callback_ = std::move(cb);
}

void AudioMonitor::fire_callback(const std::string &name, float dbfs) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (callback_)
    callback_(name, dbfs);
}

void AudioMonitor::add_source(const std::string &source_name) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entries_.count(source_name))
      return;
    entries_[source_name] = SourceEntry{};
  }
  obs_source_t *src = obs_get_source_by_name(source_name.c_str());
  if (!src) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      entries_.erase(source_name);
    }
    blog(LOG_WARNING, "[switchy] Source not found: '%s'", source_name.c_str());
    return;
  }
  auto *ctx = new SourceCallbackCtx{source_name, this};
  {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_[source_name] = {obs_source_get_weak_source(src), ctx};
  }
  obs_source_add_audio_capture_callback(src, audio_capture_cb, ctx);
  obs_source_release(src);
  blog(LOG_INFO, "[switchy] Monitoring: '%s'", source_name.c_str());
}

void AudioMonitor::remove_source(const std::string &source_name) {
  SourceEntry entry{};
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(source_name);
    if (it == entries_.end())
      return;
    entry = it->second;
    entries_.erase(it);
  }
  obs_source_t *src = obs_weak_source_get_source(entry.weak_source);
  if (src) {
    obs_source_remove_audio_capture_callback(src, audio_capture_cb, entry.ctx);
    obs_source_release(src);
  }
  obs_weak_source_release(entry.weak_source);
  delete entry.ctx;
}

void AudioMonitor::clear() {
  std::vector<std::string> names;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &kv : entries_)
      names.push_back(kv.first);
  }
  for (auto &n : names)
    remove_source(n);
}

void AudioMonitor::audio_capture_cb(void *param, obs_source_t *,
                                    const audio_data *audio, bool muted) {
  auto *ctx = static_cast<SourceCallbackCtx *>(param);
  if (!audio || !audio->frames || muted)
    return;
  float max_rms = 0.0f;
  size_t p = 0;
  while (p < MAX_AV_PLANES && audio->data[p]) {
    const float *buf = reinterpret_cast<const float *>(audio->data[p]);
    float rms = compute_rms(buf, audio->frames, 1);
    if (rms > max_rms)
      max_rms = rms;
    p++;
  }
  ctx->monitor->fire_callback(ctx->name, linear_to_dbfs(max_rms));
}
