#include "dock-widget.h"
#include "../switch-engine.h"
#include <QFrame>
#include <QHBoxLayout>
#include <algorithm>

AutoCamDock::AutoCamDock(SwitchEngine *engine, QWidget *parent)
    : QWidget(parent), engine_(engine) {
  setObjectName("PodSwitchDock");
  build_ui();
  update_timer_ = new QTimer(this);
  connect(update_timer_, &QTimer::timeout, this,
          &AutoCamDock::update_vu_meters);
  update_timer_->start(50);
}
void AutoCamDock::build_ui() {
  auto *root = new QWidget(this);
  auto *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(0, 0, 0, 0);
  main_layout->addWidget(root);
  auto *vbox = new QVBoxLayout(root);
  vbox->setSpacing(6);
  vbox->setContentsMargins(8, 8, 8, 8);
  toggle_btn_ = new QPushButton(root);
  toggle_btn_->setCheckable(true);
  set_toggle_appearance(false);
  connect(toggle_btn_, &QPushButton::clicked, this,
          &AutoCamDock::on_toggle_clicked);
  vbox->addWidget(toggle_btn_);
  auto *line = new QFrame(root);
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  vbox->addWidget(line);
  vu_container_ = new QWidget(root);
  vu_layout_ = new QVBoxLayout(vu_container_);
  vu_layout_->setSpacing(4);
  vu_layout_->setContentsMargins(0, 0, 0, 0);
  vbox->addWidget(vu_container_);
  auto *resp_row = new QHBoxLayout();
  resp_row->addWidget(new QLabel("Responsiveness:", root));
  responsiveness_combo_ = new QComboBox(root);
  responsiveness_combo_->addItem("Relaxed", (int)Responsiveness::Relaxed);
  responsiveness_combo_->addItem("Neutral", (int)Responsiveness::Neutral);
  responsiveness_combo_->addItem("Fast", (int)Responsiveness::Fast);
  responsiveness_combo_->setCurrentIndex(1);
  connect(responsiveness_combo_,
          QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
            int r = responsiveness_combo_->itemData(idx).toInt();
            engine_->set_responsiveness((Responsiveness)r);
            emit responsiveness_changed(r);
          });
  resp_row->addWidget(responsiveness_combo_);
  vbox->addLayout(resp_row);
  settings_btn_ = new QPushButton("⚙ Open Settings", root);
  connect(settings_btn_, &QPushButton::clicked, this,
          &AutoCamDock::on_settings_clicked);
  vbox->addWidget(settings_btn_);
  auto *audio_warning = new QLabel(
      "⚠ Ensure all mics are in a global audio mix\n"
      "or use the Scene Generator to auto-configure.",
      root);
  audio_warning->setStyleSheet(
      "QLabel{color:#888;font-size:10px;padding:4px 0;}");
  audio_warning->setWordWrap(true);
  vbox->addWidget(audio_warning);
  vbox->addStretch();
}
void AutoCamDock::set_toggle_appearance(bool enabled) {
  toggle_btn_->setChecked(enabled);
  if (enabled) {
    toggle_btn_->setText("● PodSwitch: ON");
    toggle_btn_->setStyleSheet(
        "QPushButton{background:#2a6e32;color:white;font-weight:bold;font-size:16px;padding:12px;border-radius:4px;}");
  } else {
    toggle_btn_->setText("○ PodSwitch: OFF");
    toggle_btn_->setStyleSheet("QPushButton{background:#444;color:#aaa;font-weight:bold;font-size:16px;padding:12px;border-radius:4px;}");
  }
}
void AutoCamDock::on_toggle_clicked() {
  bool n = !engine_->is_enabled();
  engine_->set_enabled(n);
  set_toggle_appearance(n);
  emit enabled_changed(n);
}
void AutoCamDock::set_responsiveness(Responsiveness r) {
  int idx = responsiveness_combo_->findData((int)r);
  if (idx >= 0) {
    responsiveness_combo_->blockSignals(true);
    responsiveness_combo_->setCurrentIndex(idx);
    responsiveness_combo_->blockSignals(false);
  }
}
void AutoCamDock::on_settings_clicked() { emit open_settings_requested(); }
void AutoCamDock::refresh_mappings(
    const std::vector<std::pair<std::string, std::string>> &mappings) {
  QLayoutItem *item;
  while ((item = vu_layout_->takeAt(0)) != nullptr) {
    if (item->widget())
      delete item->widget();
    delete item;
  }
  vu_rows_.clear();
  for (const auto &[src, scene] : mappings) {
    auto *rw = new QWidget(vu_container_);
    auto *hbox = new QHBoxLayout(rw);
    hbox->setContentsMargins(0, 0, 0, 0);
    auto *label = new QLabel(QString::fromStdString(src) + " → " +
                                 QString::fromStdString(scene),
                             rw);
    label->setFixedWidth(160);
    label->setToolTip(label->text());
    auto *vbars = new QVBoxLayout();
    vbars->setSpacing(2);
    auto *bar = new QProgressBar(rw);
    bar->setRange(0, 100);
    bar->setValue(0);
    bar->setTextVisible(false);
    bar->setFixedHeight(6);
    bar->setStyleSheet(
        "QProgressBar{border:1px solid #555;border-radius:2px;background:#222;}"
        "QProgressBar::chunk{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #2ecc71,stop:0.7 #f39c12,stop:1.0 #e74c3c);}");
    vbars->addWidget(bar);

    auto *motion_bar = new QProgressBar(rw);
    motion_bar->setRange(0, 100);
    motion_bar->setValue(0);
    motion_bar->setTextVisible(false);
    motion_bar->setFixedHeight(4);
    motion_bar->setStyleSheet(
        "QProgressBar{border:1px solid #555;border-radius:2px;background:#222;}"
        "QProgressBar::chunk{background:#3498db;}");
    vbars->addWidget(motion_bar);

    hbox->addWidget(label);
    hbox->addLayout(vbars, 1);
    vu_layout_->addWidget(rw);
    vu_rows_.push_back({label, bar, motion_bar, scene});
  }
}
void AutoCamDock::update_vu_meters() {
  auto levels = engine_->get_levels();
  std::string active_scene = engine_->get_current_scene();
  for (size_t i = 0; i < vu_rows_.size(); ++i) {
      if (vu_rows_[i].scene_name == active_scene && !active_scene.empty()) {
          vu_rows_[i].label->setStyleSheet("font-weight: bold; color: #2ecc71;");
      } else {
          vu_rows_[i].label->setStyleSheet("");
      }
  }
  for (const auto &level : levels) {
    for (size_t i = 0; i < vu_rows_.size(); ++i) {
      if (vu_rows_[i].label->text().startsWith(QString::fromStdString(level.source_name) + " →")) {
        int pct = (int)((std::clamp(level.audio_dbfs, -60.0f, 0.0f) + 60.0f) *
                        (100.0f / 60.0f));
        vu_rows_[i].bar->setValue(pct);
        int motion_pct = (int)(std::clamp(level.motion_energy, 0.0f, 100.0f));
        vu_rows_[i].motion_bar->setValue(motion_pct);
        break;
      }
    }
  }
}
