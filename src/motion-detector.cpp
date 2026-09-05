#include "motion-detector.h"
#include <util/base.h>
#include <algorithm>
#include <cmath>
#include <cstring>

// Downscale resolution for motion analysis — keeps CPU usage near zero
static constexpr uint32_t kThumbW = 64;
static constexpr uint32_t kThumbH = 36;

MotionDetector::MotionDetector() {}

MotionDetector::~MotionDetector() {
    clear();
}

void MotionDetector::set_callback(MotionCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(cb);
}

void MotionDetector::add_source(const std::string &source_name) {
    obs_source_t *source = obs_get_source_by_name(source_name.c_str());
    if (!source) {
        blog(LOG_WARNING, "[podswitch-motion] Source not found: %s", source_name.c_str());
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // Avoid double-registering
        for (auto &ws : watched_) {
            if (ws.name == source_name) {
                obs_source_release(source);
                return;
            }
        }
        watched_.push_back({source_name, source});
        states_[source_name] = {};
    }
    obs_source_add_video_capture_callback(source, obs_video_cb, this);
    // Keep a ref alive — released in clear()
}

void MotionDetector::clear() {
    std::vector<WatchedSource> to_clear;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        to_clear.swap(watched_);
        states_.clear();
    }
    for (auto &ws : to_clear) {
        if (ws.source) {
            obs_source_remove_video_capture_callback(ws.source, obs_video_cb, this);
            obs_source_release(ws.source);
        }
    }
}

// ─── Static OBS render-thread callback ───────────────────────────────────────
// CRITICAL: This runs on the OBS render thread. We do MINIMAL work here:
//   1. Extract source name
//   2. Downsample the Y (luma) plane to 64x36
//   3. Compute mean absolute difference against previous frame
//   4. Store result and invoke callback (cheap lambda, no locks on hot path)
void MotionDetector::obs_video_cb(void *param,
                                  obs_source_t * /*source*/,
                                  const struct obs_source_frame *frame) {
    if (!frame || !param)
        return;

    auto *self = static_cast<MotionDetector *>(param);

    // Identify which source this is via pointer match
    std::string source_name;
    {
        // brief lock just for name lookup
        std::lock_guard<std::mutex> lock(self->mutex_);
        for (auto &ws : self->watched_) {
            if (ws.source &&
                obs_get_source_by_name(ws.name.c_str()) == /*source*/ ws.source) {
                source_name = ws.name;
                break;
            }
        }
    }
    if (source_name.empty())
        return;

    // Only handle formats that have a contiguous Y plane
    if (frame->format != VIDEO_FORMAT_I420 &&
        frame->format != VIDEO_FORMAT_NV12 &&
        frame->format != VIDEO_FORMAT_I444 &&
        frame->format != VIDEO_FORMAT_YVYU &&
        frame->format != VIDEO_FORMAT_YUY2 &&
        frame->format != VIDEO_FORMAT_UYVY)
        return;

    const uint32_t fw = frame->width;
    const uint32_t fh = frame->height;
    if (fw == 0 || fh == 0)
        return;

    // Downsample luma to kThumbW x kThumbH using nearest-neighbour
    std::vector<uint8_t> thumb(kThumbW * kThumbH);
    const uint8_t *src = frame->data[0];
    const uint32_t stride = frame->linesize[0];
    for (uint32_t ty = 0; ty < kThumbH; ++ty) {
        uint32_t sy = (ty * fh) / kThumbH;
        for (uint32_t tx = 0; tx < kThumbW; ++tx) {
            uint32_t sx = (tx * fw) / kThumbW;
            thumb[ty * kThumbW + tx] = src[sy * stride + sx];
        }
    }

    float energy = 0.0f;
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        auto &state = self->states_[source_name];
        if (state.prev_luma.size() == thumb.size()) {
            // Compute Mean Absolute Difference (MAD) across all pixels
            uint64_t sum = 0;
            for (size_t i = 0; i < thumb.size(); ++i) {
                int diff = (int)thumb[i] - (int)state.prev_luma[i];
                sum += (uint64_t)std::abs(diff);
            }
            float mad = (float)sum / (float)thumb.size();
            // Scale to 0-100 range: MAD of 25 luma units = 100% energy
            energy = std::min(mad * (100.0f / 25.0f), 100.0f);
        }
        state.prev_luma = std::move(thumb);

        // Fire callback while lock is held (callback is a lightweight store)
        if (self->callback_)
            self->callback_(source_name, energy);
    }
}
