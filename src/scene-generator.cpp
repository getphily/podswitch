#include "scene-generator.h"
#include <obs.h>
#include <obs-frontend-api.h>

// Names we manage — used for clean-slate deletion
static const char *kGeneratedScenes[] = {
    "🟢 Starting Soon", "👤 Person 1", "👤 Person 2", "👤 Person 3", "👤 Person 4",
    "👥 Split Screen", "🔴 Thank You", "--- INPUTS ---",
    "📺 Inputs / Person 1", "📺 Inputs / Person 2", "📺 Inputs / Person 3", "📺 Inputs / Person 4",
    "\U0001F399\uFE0F Live Audio Mix", 
    // Legacy names from v1.0.0 for proper cleanup
    "Fallback", "Starting Soon", "Person 1", "Person 2", "Person 3", "Person 4",
    "Split Screen", "Thank You", "Inputs / Person 1", "Inputs / Person 2", "Inputs / Person 3", "Inputs / Person 4",
    "Person 1 Input", "Person 2 Input", "Person 3 Input", "Person 4 Input",
    "Person 1 Solo", "Person 2 Solo", "Person 3 Solo", "Person 4 Solo",
    "Grid Scene", nullptr
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

static void add_video_to_scene(obs_scene_t *scene,
                               const std::string &source_name,
                               bool full_screen,
                               float pos_x = 0, float pos_y = 0,
                               float region_w = -1, float region_h = -1) {
    obs_source_t *video_source = obs_get_source_by_name(source_name.c_str());
    if (!video_source)
        return;

    obs_sceneitem_t *item = obs_scene_add(scene, video_source);

    uint32_t base_w = obs_source_get_width(video_source);
    uint32_t base_h = obs_source_get_height(video_source);

    if (item && base_w > 0 && base_h > 0) {
        float canvas_w = 1920.0f;
        float canvas_h = 1080.0f;

        if (region_w < 0) region_w = canvas_w;
        if (region_h < 0) region_h = canvas_h;

        if (full_screen) {
            float sx = canvas_w / (float)base_w;
            float sy = canvas_h / (float)base_h;
            float s  = (sx < sy) ? sx : sy;
            struct vec2 scale; vec2_set(&scale, s, s);
            struct vec2 pos;
            vec2_set(&pos,
                     (canvas_w - (float)base_w * s) / 2.0f,
                     (canvas_h - (float)base_h * s) / 2.0f);
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

static void add_audio_mix_to_scene(obs_scene_t *scene, obs_source_t *audio_mix) {
    if (audio_mix)
        obs_scene_add(scene, audio_mix);
}

static void populate_static_scene(obs_scene_t *scene, const char *display_text) {
    obs_data_t *settings = obs_data_create();
    obs_data_set_int(settings, "color", 0xFF202020);
    obs_data_set_int(settings, "width", 1920);
    obs_data_set_int(settings, "height", 1080);
    obs_source_t *color_bg = obs_source_create("color_source", (std::string(obs_source_get_name(obs_scene_get_source(scene))) + " BG").c_str(), settings, nullptr);
    obs_data_release(settings);
    
    if (color_bg) {
        obs_scene_add(scene, color_bg);
        obs_source_release(color_bg);
    }
    
    if (display_text) {
        obs_data_t *text_settings = obs_data_create();
        obs_data_set_string(text_settings, "text", display_text);
        obs_source_t *text_src = obs_source_create("text_ft2_source_v2", (std::string(obs_source_get_name(obs_scene_get_source(scene))) + " Text").c_str(), text_settings, nullptr);
        obs_data_release(text_settings);
        if (text_src) {
            obs_sceneitem_t *item = obs_scene_add(scene, text_src);
            struct vec2 pos; vec2_set(&pos, 200, 200);
            obs_sceneitem_set_pos(item, &pos);
            obs_source_release(text_src);
        }
    }
}

bool SceneGenerator::generate(const SceneGenSettings &settings) {
    delete_existing_generated_scenes();

    int num_people = 2;
    if (settings.format == PodcastFormat::ThreePerson) num_people = 3;
    if (settings.format == PodcastFormat::FourPerson) num_people = 4;

    // 1. Create Scenes in exact reverse order so they stack logically top-to-bottom in OBS
    obs_scene_t *audio_scene = obs_scene_create("\U0001F399\uFE0F Live Audio Mix");
    obs_source_t *audio_mix = obs_scene_get_source(audio_scene);

    std::vector<obs_scene_t*> inputs(num_people);
    for (int i = num_people; i >= 1; --i) {
        inputs[i-1] = obs_scene_create(("📺 Inputs / Person " + std::to_string(i)).c_str());
    }

    obs_scene_t *scene_inputs_folder = obs_scene_create("--- INPUTS ---");
    obs_scene_t *scene_thankyou = obs_scene_create("🔴 Thank You");
    obs_scene_t *scene_grid = obs_scene_create("👥 Split Screen");

    std::vector<obs_scene_t*> solos(num_people);
    for (int i = num_people; i >= 1; --i) {
        solos[i-1] = obs_scene_create(("👤 Person " + std::to_string(i)).c_str());
    }

    obs_scene_t *scene_starting = obs_scene_create("🟢 Starting Soon");

    // 2. Populate Audio Mix
    for (const auto &name : settings.audio_sources) {
        obs_source_t *source = obs_get_source_by_name(name.c_str());
        if (source) {
            obs_scene_add(audio_scene, source);
            obs_source_release(source);
        }
    }

    // 3. Populate Inputs
    for (int i = 0; i < num_people; ++i) {
        populate_static_scene(inputs[i], ("Add Camera " + std::to_string(i+1) + " Here").c_str());
    }

    // 4. Populate Separator
    populate_static_scene(scene_inputs_folder, nullptr);

    // 5. Populate Thank You
    populate_static_scene(scene_thankyou, "Thanks for Watching");

    // 7. Populate Split Screen
    if (num_people == 2) {
        add_video_to_scene(scene_grid, "📺 Inputs / Person 1", false, 0,   0, 960, 1080);
        add_video_to_scene(scene_grid, "📺 Inputs / Person 2", false, 960, 0, 960, 1080);
    } else if (num_people == 3) {
        add_video_to_scene(scene_grid, "📺 Inputs / Person 1", false, 0,    0, 640, 1080);
        add_video_to_scene(scene_grid, "📺 Inputs / Person 2", false, 640,  0, 640, 1080);
        add_video_to_scene(scene_grid, "📺 Inputs / Person 3", false, 1280, 0, 640, 1080);
    } else if (num_people == 4) {
        add_video_to_scene(scene_grid, "📺 Inputs / Person 1", false, 0,   0,   960, 540);
        add_video_to_scene(scene_grid, "📺 Inputs / Person 2", false, 960, 0,   960, 540);
        add_video_to_scene(scene_grid, "📺 Inputs / Person 3", false, 0,   540, 960, 540);
        add_video_to_scene(scene_grid, "📺 Inputs / Person 4", false, 960, 540, 960, 540);
    }
    add_audio_mix_to_scene(scene_grid, audio_mix);

    // 8. Populate Solos
    for (int i = 0; i < num_people; ++i) {
        add_video_to_scene(solos[i], "📺 Inputs / Person " + std::to_string(i+1), true);
        add_audio_mix_to_scene(solos[i], audio_mix);
    }

    // 9. Populate Starting Soon
    populate_static_scene(scene_starting, "Starting Soon...");

    // 9. Release references
    obs_scene_release(scene_starting);
    obs_scene_release(scene_grid);
    obs_scene_release(scene_thankyou);
    obs_scene_release(scene_inputs_folder);
    obs_scene_release(audio_scene);
    for (auto s : solos) obs_scene_release(s);
    for (auto s : inputs) obs_scene_release(s);

    return true;
}
