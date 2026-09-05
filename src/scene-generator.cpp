#include "scene-generator.h"
#include <obs.h>

void SceneGenerator::delete_existing_generated_scenes() {
    // This function will iterate through scenes and delete those we generated.
    // For now, we will identify them by name prefix or suffix: " (PodSwitch)"
    // Or we just hardcode the names we generate.
    const char* generated_names[] = {
        "Host 1 Solo", "Guest 1 Solo", "Guest 2 Solo", "Split Screen", "Wide Shot", "🎙️ Live Audio Mix"
    };
    for (const char* name : generated_names) {
        obs_source_t* source = obs_get_source_by_name(name);
        if (source) {
            obs_source_remove(source);
            obs_source_release(source);
        }
    }
}

void* SceneGenerator::create_audio_mix_scene(const std::vector<std::string> &audio_sources) {
    obs_scene_t* audio_scene = obs_scene_create("🎙️ Live Audio Mix");
    if (!audio_scene) return nullptr;

    for (const auto& name : audio_sources) {
        obs_source_t* source = obs_get_source_by_name(name.c_str());
        if (source) {
            obs_scene_add(audio_scene, source);
            obs_source_release(source);
        }
    }
    obs_source_t *audio_source_out = obs_scene_get_source(audio_scene);
    obs_scene_release(audio_scene); // Scene keeps itself alive while in the frontend? Actually, releasing it transfers ownership to frontend?
    return audio_source_out;
}

static void add_source_to_scene(obs_scene_t* scene, const std::string& source_name, obs_source_t* audio_mix, bool full_screen, float pos_x = 0, float pos_y = 0, float width = 1920, float height = 1080) {
    // Add video
    obs_source_t* video_source = obs_get_source_by_name(source_name.c_str());
    if (video_source) {
        obs_sceneitem_t* item = obs_scene_add(scene, video_source);
        
        uint32_t base_w = obs_source_get_width(video_source);
        uint32_t base_h = obs_source_get_height(video_source);
        if (base_w > 0 && base_h > 0) {
            struct vec2 scale;
            vec2_set(&scale, 1.0f, 1.0f);
            
            if (full_screen) {
                // Scale to fit 1920x1080
                float scale_x = 1920.0f / (float)base_w;
                float scale_y = 1080.0f / (float)base_h;
                float s = (scale_x < scale_y) ? scale_x : scale_y;
                vec2_set(&scale, s, s);
                
                struct vec2 pos;
                vec2_set(&pos, (1920.0f - (base_w * s)) / 2.0f, (1080.0f - (base_h * s)) / 2.0f);
                obs_sceneitem_set_pos(item, &pos);
                obs_sceneitem_set_scale(item, &scale);
            } else {
                // Split screen cropping
                // We want to fill 'width' x 'height'
                float target_aspect = width / height;
                float src_aspect = (float)base_w / (float)base_h;
                
                obs_sceneitem_crop crop = {0};
                if (src_aspect > target_aspect) {
                    // Source is too wide, crop left/right
                    float crop_pixels = base_w - (base_h * target_aspect);
                    crop.left = (int)(crop_pixels / 2.0f);
                    crop.right = (int)(crop_pixels / 2.0f);
                } else {
                    // Source is too tall, crop top/bottom
                    float crop_pixels = base_h - (base_w / target_aspect);
                    crop.top = (int)(crop_pixels / 2.0f);
                    crop.bottom = (int)(crop_pixels / 2.0f);
                }
                obs_sceneitem_set_crop(item, &crop);
                
                float cropped_w = base_w - crop.left - crop.right;
                float scale_xy = width / cropped_w;
                vec2_set(&scale, scale_xy, scale_xy);
                obs_sceneitem_set_scale(item, &scale);
                
                struct vec2 pos;
                vec2_set(&pos, pos_x, pos_y);
                obs_sceneitem_set_pos(item, &pos);
            }
        }
        obs_source_release(video_source);
    }
    
    // Add audio mix
    if (audio_mix) {
        obs_scene_add(scene, audio_mix);
    }
}

void SceneGenerator::generate_1_on_1(const SceneGenSettings &settings, void *audio_scene_source) {
    obs_source_t* audio_mix = (obs_source_t*)audio_scene_source;
    
    // Host Solo
    obs_scene_t* host_scene = obs_scene_create("Host 1 Solo");
    add_source_to_scene(host_scene, settings.host1_source, audio_mix, true);
    obs_scene_release(host_scene);
    
    // Guest Solo
    obs_scene_t* guest_scene = obs_scene_create("Guest 1 Solo");
    add_source_to_scene(guest_scene, settings.guest1_source, audio_mix, true);
    obs_scene_release(guest_scene);
    
    // Split Screen
    obs_scene_t* split_scene = obs_scene_create("Split Screen");
    add_source_to_scene(split_scene, settings.host1_source, audio_mix, false, 0, 0, 960, 1080);
    add_source_to_scene(split_scene, settings.guest1_source, audio_mix, false, 960, 0, 960, 1080);
    obs_scene_release(split_scene);
}

void SceneGenerator::generate_power_dynamic(const SceneGenSettings &settings, void *audio_scene_source) {
    obs_source_t* audio_mix = (obs_source_t*)audio_scene_source;
    
    // Solos
    obs_scene_t* host_scene = obs_scene_create("Host 1 Solo");
    add_source_to_scene(host_scene, settings.host1_source, audio_mix, true);
    obs_scene_release(host_scene);
    
    obs_scene_t* g1_scene = obs_scene_create("Guest 1 Solo");
    add_source_to_scene(g1_scene, settings.guest1_source, audio_mix, true);
    obs_scene_release(g1_scene);
    
    obs_scene_t* g2_scene = obs_scene_create("Guest 2 Solo");
    add_source_to_scene(g2_scene, settings.guest2_source, audio_mix, true);
    obs_scene_release(g2_scene);
    
    // Asymmetrical Split
    obs_scene_t* split_scene = obs_scene_create("Split Screen");
    add_source_to_scene(split_scene, settings.host1_source, audio_mix, false, 0, 0, 960, 1080);
    add_source_to_scene(split_scene, settings.guest1_source, audio_mix, false, 960, 0, 960, 540);
    add_source_to_scene(split_scene, settings.guest2_source, audio_mix, false, 960, 540, 960, 540);
    obs_scene_release(split_scene);
}

bool SceneGenerator::generate(const SceneGenSettings &settings) {
    delete_existing_generated_scenes();
    
    obs_source_t* audio_mix = (obs_source_t*)create_audio_mix_scene(settings.audio_sources);
    
    if (settings.format == PodcastFormat::OneOnOne) {
        generate_1_on_1(settings, audio_mix);
    } else {
        generate_power_dynamic(settings, audio_mix);
    }
    
    return true;
}
