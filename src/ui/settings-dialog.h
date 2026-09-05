#pragma once
#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QTabWidget>
#include <QFormLayout>
#include <vector>
#include "../switch-engine.h"
#include "../config.h"

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(Config *config, QWidget *parent=nullptr);
    ~SettingsDialog() override=default;
    void populate_sources(const std::vector<std::string> &audio_sources,
                          const std::vector<std::string> &video_sources,
                          const std::vector<std::string> &scene_names);
signals:
    void settings_applied(const Config &config);
private slots:
    void on_add_mapping(); void on_remove_mapping(); void on_ok(); void on_apply();
    void on_generate_scenes();
private:
    Config *config_;
    QTableWidget *mappings_table_;
    QPushButton *add_btn_, *remove_btn_;
    QTabWidget *tab_widget_;
    
    // Generator tab fields
    QComboBox *gen_format_combo_;
    QComboBox *gen_host1_combo_, *gen_guest1_combo_, *gen_guest2_combo_;
    QPushButton *gen_btn_;
    
    QRadioButton *relaxed_radio_, *neutral_radio_, *fast_radio_;
    QRadioButton *mi_off_radio_, *mi_moderate_radio_, *mi_high_radio_;
    QSpinBox *hold_time_spin_, *fade_duration_spin_;
    QComboBox *fallback_combo_;
    QCheckBox *fade_check_;
    std::vector<std::string> audio_sources_, video_sources_, scene_names_;
    void build_ui(); void load_from_config(); void save_to_config();
    void add_mapping_row(const CamMapping &m={});
    bool config_loaded_ = false;
};
