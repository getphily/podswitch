#pragma once
#include <string>
#include <vector>

enum class PodcastFormat {
    OneOnOne,
    PowerDynamic
};

struct SceneGenSettings {
    PodcastFormat format = PodcastFormat::OneOnOne;
    std::string host1_source;
    std::string guest1_source;
    std::string guest2_source; // For Power Dynamic
    std::vector<std::string> audio_sources;
};

// Forward declare OBS types to avoid pulling obs.h into every file that
// includes this header.
struct obs_source;
typedef struct obs_source obs_source_t;
struct obs_scene;
typedef struct obs_scene obs_scene_t;

class SceneGenerator {
public:
    static bool generate(const SceneGenSettings &settings);
private:
    static void delete_existing_generated_scenes();
    static obs_scene_t *create_audio_mix_scene(const std::vector<std::string> &audio_sources);
    static void generate_1_on_1(const SceneGenSettings &settings, obs_source_t *audio_mix);
    static void generate_power_dynamic(const SceneGenSettings &settings, obs_source_t *audio_mix);
};
