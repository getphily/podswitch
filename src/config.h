#pragma once
#include <string>
#include <vector>
#include "switch-engine.h"

class Config {
public:
    Config(); ~Config();
    std::vector<CamMapping> get_mappings() const { return mappings_; }
    void set_mappings(const std::vector<CamMapping> &m) { mappings_ = m; }
    Responsiveness get_responsiveness() const { return responsiveness_; }
    void set_responsiveness(Responsiveness r) { responsiveness_ = r; }
    MotionInfluence get_motion_influence() const { return motion_influence_; }
    void set_motion_influence(MotionInfluence m) { motion_influence_ = m; }
    ReactionCutaways get_reaction_cutaways() const { return reaction_cutaways_; }
    void set_reaction_cutaways(ReactionCutaways r) { reaction_cutaways_ = r; }
    int get_gen_format() const { return gen_format_; }
    void set_gen_format(int f) { gen_format_ = f; }

    void load(); void save() const;
private:
    std::vector<CamMapping> mappings_;
    Responsiveness responsiveness_ = Responsiveness::Neutral;
    MotionInfluence motion_influence_ = MotionInfluence::Moderate;
    ReactionCutaways reaction_cutaways_ = ReactionCutaways::Never;
    int gen_format_ = 0;

    std::string get_config_path() const;
};
