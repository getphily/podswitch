#pragma once
#include <obs.h>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

using AudioLevelCallback = std::function<void(const std::string &source_name, float dbfs)>;
class AudioMonitor;

struct SourceCallbackCtx {
    std::string name;
    AudioMonitor *monitor = nullptr;
};

class AudioMonitor {
public:
    AudioMonitor();
    ~AudioMonitor();
    void add_source(const std::string &source_name);
    void remove_source(const std::string &source_name);
    void clear();
    void set_callback(AudioLevelCallback cb);
    void fire_callback(const std::string &name, float dbfs);
private:
    struct SourceEntry {
        obs_weak_source_t *weak_source = nullptr;
        SourceCallbackCtx *ctx = nullptr;
    };
    static void audio_capture_cb(void *param, obs_source_t *,
                                 const audio_data *audio, bool muted);
    std::mutex mutex_;
    std::unordered_map<std::string, SourceEntry> entries_;
    std::unordered_set<std::string> active_sources_;
    AudioLevelCallback callback_;
};
