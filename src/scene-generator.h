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

class SceneGenerator {
public:
    static bool generate(const SceneGenSettings &settings);
private:
    static void delete_existing_generated_scenes();
    static void generate_1_on_1(const SceneGenSettings &settings, void *audio_scene_source);
    static void generate_power_dynamic(const SceneGenSettings &settings, void *audio_scene_source);
    static void *create_audio_mix_scene(const std::vector<std::string> &audio_sources);
};
