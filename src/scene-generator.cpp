#include "scene-generator.h"
#include <obs.h>
#include <obs-frontend-api.h>

// Names we manage — used for clean-slate deletion
static const char *kGeneratedScenes[] = {
    "Host 1 Solo", "Guest 1 Solo", "Guest 2 Solo",
    "Split Screen", "\U0001F399\uFE0F Live Audio Mix", nullptr
};

void SceneGenerator::delete_existing_generated_scenes() {
    for (int i = 0; kGeneratedScenes[i] != nullptr; ++i) {
        obs_source_t *source = obs_get_source_by_name(kGeneratedScenes[i]);
        if (source) {
            obs_source_remove(source);
            obs_source_release(source);
        }
    }
}

// Returns the obs_source_t* for the audio mix scene (borrowed ref).
obs_source_t *SceneGenerator::create_audio_mix_scene(
    const std::vector<std::string> &audio_sources) {
    obs_scene_t *audio_scene = obs_scene_create("\U0001F399\uFE0F Live Audio Mix");
    if (!audio_scene)
        return nullptr;

    for (const auto &name : audio_sources) {
        obs_source_t *source = obs_get_source_by_name(name.c_str());
        if (source) {
            obs_scene_add(audio_scene, source);
            obs_source_release(source);
        }
    }
    // obs_scene_get_source returns a borrowed ref kept alive by the scene object
    obs_source_t *out = obs_scene_get_source(audio_scene);
    obs_scene_release(audio_scene);
    return out;
}

// Adds a single video source into a scene with aspect-ratio-aware scaling.
// Audio is NOT added here — it is added once per scene after all video sources.
static void add_video_to_scene(obs_scene_t *scene,
                               const std::string &source_name,
                               bool full_screen,
                               float pos_x = 0, float pos_y = 0,
                               float region_w = 1920, float region_h = 1080) {
    obs_source_t *video_source = obs_get_source_by_name(source_name.c_str());
    if (!video_source)
        return;

    obs_sceneitem_t *item = obs_scene_add(scene, video_source);

    uint32_t base_w = obs_source_get_width(video_source);
    uint32_t base_h = obs_source_get_height(video_source);

    if (item && base_w > 0 && base_h > 0) {
        if (full_screen) {
            float sx = 1920.0f / (float)base_w;
            float sy = 1080.0f / (float)base_h;
            float s  = (sx < sy) ? sx : sy;
            struct vec2 scale; vec2_set(&scale, s, s);
            struct vec2 pos;
            vec2_set(&pos,
                     (1920.0f - (float)base_w * s) / 2.0f,
                     (1080.0f - (float)base_h * s) / 2.0f);
            obs_sceneitem_set_pos(item, &pos);
            obs_sceneitem_set_scale(item, &scale);
        } else {
            float target_aspect = region_w / region_h;
            float src_aspect    = (float)base_w / (float)base_h;

            obs_sceneitem_crop crop = {};
            if (src_aspect > target_aspect) {
                float px = base_w - (base_h * target_aspect);
                crop.left  = (int)(px / 2.0f);
                crop.right = (int)(px / 2.0f);
            } else {
                float py = base_h - (base_w / target_aspect);
                crop.top    = (int)(py / 2.0f);
                crop.bottom = (int)(py / 2.0f);
            }
            obs_sceneitem_set_crop(item, &crop);

            float cropped_w = (float)base_w - crop.left - crop.right;
            float s = region_w / cropped_w;
            struct vec2 scale; vec2_set(&scale, s, s);
            obs_sceneitem_set_scale(item, &scale);

            struct vec2 pos; vec2_set(&pos, pos_x, pos_y);
            obs_sceneitem_set_pos(item, &pos);
        }
    }
    obs_source_release(video_source);
}

// Nests the live audio mix source into a scene exactly once.
static void add_audio_mix_to_scene(obs_scene_t *scene,
                                   obs_source_t *audio_mix) {
    if (audio_mix)
        obs_scene_add(scene, audio_mix);
}

void SceneGenerator::generate_1_on_1(const SceneGenSettings &settings,
                                     obs_source_t *audio_mix) {
    // Host 1 Solo
    obs_scene_t *host_scene = obs_scene_create("Host 1 Solo");
    add_video_to_scene(host_scene, settings.host1_source, true);
    add_audio_mix_to_scene(host_scene, audio_mix);
    obs_scene_release(host_scene);

    // Guest 1 Solo
    obs_scene_t *guest_scene = obs_scene_create("Guest 1 Solo");
    add_video_to_scene(guest_scene, settings.guest1_source, true);
    add_audio_mix_to_scene(guest_scene, audio_mix);
    obs_scene_release(guest_scene);

    // 50/50 Split Screen
    obs_scene_t *split_scene = obs_scene_create("Split Screen");
    add_video_to_scene(split_scene, settings.host1_source,  false, 0,   0, 960, 1080);
    add_video_to_scene(split_scene, settings.guest1_source, false, 960, 0, 960, 1080);
    add_audio_mix_to_scene(split_scene, audio_mix);
    obs_scene_release(split_scene);
}

void SceneGenerator::generate_power_dynamic(const SceneGenSettings &settings,
                                            obs_source_t *audio_mix) {
    // Solos
    obs_scene_t *host_scene = obs_scene_create("Host 1 Solo");
    add_video_to_scene(host_scene, settings.host1_source, true);
    add_audio_mix_to_scene(host_scene, audio_mix);
    obs_scene_release(host_scene);

    obs_scene_t *g1_scene = obs_scene_create("Guest 1 Solo");
    add_video_to_scene(g1_scene, settings.guest1_source, true);
    add_audio_mix_to_scene(g1_scene, audio_mix);
    obs_scene_release(g1_scene);

    obs_scene_t *g2_scene = obs_scene_create("Guest 2 Solo");
    add_video_to_scene(g2_scene, settings.guest2_source, true);
    add_audio_mix_to_scene(g2_scene, audio_mix);
    obs_scene_release(g2_scene);

    // Asymmetrical Split: Host 50% left, Guests stacked 25% each on right
    obs_scene_t *split_scene = obs_scene_create("Split Screen");
    add_video_to_scene(split_scene, settings.host1_source,  false, 0,   0,   960, 1080);
    add_video_to_scene(split_scene, settings.guest1_source, false, 960, 0,   960, 540);
    add_video_to_scene(split_scene, settings.guest2_source, false, 960, 540, 960, 540);
    add_audio_mix_to_scene(split_scene, audio_mix);
    obs_scene_release(split_scene);
}

bool SceneGenerator::generate(const SceneGenSettings &settings) {
    delete_existing_generated_scenes();

    obs_source_t *audio_mix = create_audio_mix_scene(settings.audio_sources);

    if (settings.format == PodcastFormat::OneOnOne)
        generate_1_on_1(settings, audio_mix);
    else
        generate_power_dynamic(settings, audio_mix);

    return true;
}
