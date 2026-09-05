#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <map>
#include <obs.h>

// Callback: source_name -> motion_energy (0.0 - 100.0)
using MotionCallback = std::function<void(const std::string &source_name, float energy)>;

struct MotionState {
    std::vector<uint8_t> prev_luma; // Previous frame luma at reduced resolution
    uint32_t w = 0;                 // Downscaled width
    uint32_t h = 0;                 // Downscaled height
};

class MotionDetector {
public:
    MotionDetector();
    ~MotionDetector();

    void set_callback(MotionCallback cb);
    // Start watching a video source for motion. Registers OBS video capture callback.
    void add_source(const std::string &source_name);
    // Stop watching all sources.
    void clear();

private:
    // Static OBS video capture callback (called on OBS render thread)
    static void obs_video_cb(void *param,
                             obs_source_t *source,
                             const struct obs_source_frame *frame);

    // Per-source state, keyed by source name
    mutable std::mutex mutex_;
    std::map<std::string, MotionState> states_;
    MotionCallback callback_;

    struct WatchedSource {
        std::string name;
        obs_source_t *source = nullptr;
    };
    std::vector<WatchedSource> watched_;
};
