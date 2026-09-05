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
    std::vector<uint8_t> curr_luma; // Reusable buffer for current frame
    std::vector<uint8_t> prev_luma; // Previous frame luma at reduced resolution
    int frame_skip_counter = 0;
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
    void process_frame(const std::string &source_name, const struct obs_source_frame *frame);

private:

    // Per-source state, keyed by source name
    mutable std::mutex mutex_;
    std::map<std::string, MotionState> states_;
    MotionCallback callback_;

    struct WatchedSource {
        std::string name;
        obs_weak_source_t *weak_source = nullptr;
        obs_weak_source_t *weak_filter = nullptr;
    };
    std::vector<WatchedSource> watched_;
};
