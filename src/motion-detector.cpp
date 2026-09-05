#include "motion-detector.h"
#include <util/base.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <obs-module.h>

static constexpr uint32_t kThumbW = 64;
static constexpr uint32_t kThumbH = 36;

struct MotionFilterData {
    MotionDetector *detector = nullptr;
    std::string source_name;
};

static const char *motion_filter_get_name(void *) { return "PodSwitch Motion Tracker"; }

static void *motion_filter_create(obs_data_t *settings, obs_source_t *context) {
    auto *data = new MotionFilterData();
    data->detector = (MotionDetector *)obs_data_get_int(settings, "motion_detector_ptr");
    data->source_name = obs_data_get_string(settings, "source_name");
    return data;
}

static void motion_filter_update(void *data_ptr, obs_data_t *settings) {
    auto *data = (MotionFilterData *)data_ptr;
    data->detector = (MotionDetector *)obs_data_get_int(settings, "motion_detector_ptr");
    data->source_name = obs_data_get_string(settings, "source_name");
}

static void motion_filter_destroy(void *data_ptr) {
    delete (MotionFilterData *)data_ptr;
}

static struct obs_source_frame *motion_filter_video(void *data_ptr, struct obs_source_frame *frame) {
    auto *data = (MotionFilterData *)data_ptr;
    if (data->detector) {
        data->detector->process_frame(data->source_name, frame);
    }
    return frame;
}

struct obs_source_info motion_filter_info = {};

void podswitch_register_motion_filter() {
    motion_filter_info.id = "podswitch_motion_filter";
    motion_filter_info.type = OBS_SOURCE_TYPE_FILTER;
    motion_filter_info.output_flags = OBS_SOURCE_VIDEO;
    motion_filter_info.get_name = motion_filter_get_name;
    motion_filter_info.create = motion_filter_create;
    motion_filter_info.destroy = motion_filter_destroy;
    motion_filter_info.update = motion_filter_update;
    motion_filter_info.filter_video = motion_filter_video;
    obs_register_source(&motion_filter_info);
}

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
    
    obs_weak_source_t *weak = obs_source_get_weak_source(source);
    obs_weak_source_t *weak_filter = nullptr;

    obs_source_t *filter = obs_source_create("podswitch_motion_filter", "PodSwitch Motion", nullptr, nullptr);
    if (filter) {
        obs_data_t *settings = obs_data_create();
        obs_data_set_int(settings, "motion_detector_ptr", (long long)this);
        obs_data_set_string(settings, "source_name", source_name.c_str());
        obs_source_update(filter, settings);
        obs_data_release(settings);
        
        obs_source_filter_add(source, filter);
        weak_filter = obs_source_get_weak_source(filter);
        obs_source_release(filter);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        watched_.push_back({source_name, weak, weak_filter});
        states_[source_name] = {};
    }
    obs_source_release(source);
}

void MotionDetector::clear() {
    std::vector<WatchedSource> to_clear;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        to_clear.swap(watched_);
        states_.clear();
    }
    for (auto &ws : to_clear) {
        obs_source_t *source = nullptr;
        obs_source_t *filter = nullptr;
        
        if (ws.weak_source) source = obs_weak_source_get_source(ws.weak_source);
        if (ws.weak_filter) filter = obs_weak_source_get_source(ws.weak_filter);
        
        if (source && filter) {
            obs_source_filter_remove(source, filter);
        }
        
        if (source) obs_source_release(source);
        if (filter) obs_source_release(filter);
        
        if (ws.weak_source) obs_weak_source_release(ws.weak_source);
        if (ws.weak_filter) obs_weak_source_release(ws.weak_filter);
    }
}

void MotionDetector::process_frame(const std::string &source_name, const struct obs_source_frame *frame) {
    if (!frame) return;

    if (frame->format != VIDEO_FORMAT_I420 &&
        frame->format != VIDEO_FORMAT_NV12 &&
        frame->format != VIDEO_FORMAT_I444 &&
        frame->format != VIDEO_FORMAT_YVYU &&
        frame->format != VIDEO_FORMAT_YUY2 &&
        frame->format != VIDEO_FORMAT_UYVY)
        return;

    const uint32_t fw = frame->width;
    const uint32_t fh = frame->height;
    if (fw == 0 || fh == 0) return;

    int luma_offset = 0;
    int pixel_stride = 1;
    if (frame->format == VIDEO_FORMAT_YUY2 || frame->format == VIDEO_FORMAT_YVYU) {
        pixel_stride = 2;
        luma_offset = 0;
    } else if (frame->format == VIDEO_FORMAT_UYVY) {
        pixel_stride = 2;
        luma_offset = 1;
    }

    std::array<uint8_t, kThumbW * kThumbH> thumb;
    const uint8_t *src = frame->data[0];
    const uint32_t stride = frame->linesize[0];
    for (uint32_t ty = 0; ty < kThumbH; ++ty) {
        uint32_t sy = (ty * fh) / kThumbH;
        for (uint32_t tx = 0; tx < kThumbW; ++tx) {
            uint32_t sx = (tx * fw) / kThumbW;
            thumb[ty * kThumbW + tx] = src[sy * stride + sx * pixel_stride + luma_offset];
        }
    }

    float energy = 0.0f;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto &state = states_[source_name];
        
        // Frame skip: only process every 3rd frame (~20fps for a 60fps source)
        if (++state.frame_skip_counter < 3) {
            return;
        }
        state.frame_skip_counter = 0;

        if (state.prev_luma.size() == thumb.size()) {
            uint64_t sum = 0;
            for (size_t i = 0; i < thumb.size(); ++i) {
                int diff = (int)thumb[i] - (int)state.prev_luma[i];
                sum += (uint64_t)std::abs(diff);
            }
            float mad = (float)sum / (float)thumb.size();
            energy = std::min(mad * (100.0f / 25.0f), 100.0f);
        }
        
        // Store into state.prev_luma (resize if needed)
        if (state.prev_luma.size() != thumb.size()) {
            state.prev_luma.resize(thumb.size());
        }
        std::copy(thumb.begin(), thumb.end(), state.prev_luma.begin());
        
        if (callback_) callback_(source_name, energy);
    }
}
