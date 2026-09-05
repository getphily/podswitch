#pragma once
#include <QDockWidget>
#include <QTimer>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QComboBox>
#include <QWidget>
#include <vector>
#include <string>
#include "config.h"
class SwitchEngine;
class AutoCamDock : public QWidget {
    Q_OBJECT
public:
    explicit AutoCamDock(SwitchEngine *engine, QWidget *parent=nullptr);
    ~AutoCamDock() override=default;
    void refresh_mappings(const std::vector<std::pair<std::string,std::string>> &mappings);
    void set_responsiveness(Responsiveness r);
signals:
    void open_settings_requested();
    void enabled_changed(bool enabled);
    void responsiveness_changed(int r);
private slots:
    void on_toggle_clicked(); void on_settings_clicked(); void update_vu_meters();
private:
    SwitchEngine *engine_;
    QPushButton *toggle_btn_, *settings_btn_;
    QComboBox *responsiveness_combo_;
    QWidget *vu_container_; QVBoxLayout *vu_layout_;
    struct VURow{QLabel *label;QProgressBar *bar;};
    std::vector<VURow> vu_rows_;
    QTimer *update_timer_;
    void build_ui(); void set_toggle_appearance(bool enabled);
};
