#include "settings-dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(Config *config, QWidget *parent)
    : QDialog(parent), config_(config) {
  setWindowTitle("PodSwitch — Settings");
  setMinimumWidth(580);
  build_ui();
}
void SettingsDialog::populate_sources(
    const std::vector<std::string> &audio_sources,
    const std::vector<std::string> &video_sources,
    const std::vector<std::string> &scene_names) {
  audio_sources_ = audio_sources;
  video_sources_ = video_sources;
  scene_names_ = scene_names;
  for (int row = 0; row < mappings_table_->rowCount(); ++row) {
    auto *sc = qobject_cast<QComboBox *>(mappings_table_->cellWidget(row, 0));
    auto *vc = qobject_cast<QComboBox *>(mappings_table_->cellWidget(row, 1));
    auto *ss = qobject_cast<QComboBox *>(mappings_table_->cellWidget(row, 2));
    if (!sc || !vc || !ss)
      continue;
    QString cs = sc->currentText(), cv = vc->currentText(), csn = ss->currentText();
    sc->clear();
    vc->clear();
    ss->clear();
    for (auto &s : audio_sources_)
      sc->addItem(QString::fromStdString(s));
    vc->addItem("— None —");
    for (auto &s : video_sources_)
      vc->addItem(QString::fromStdString(s));
    for (auto &s : scene_names_)
      ss->addItem(QString::fromStdString(s));
    sc->setCurrentText(cs);
    vc->setCurrentText(cv);
    ss->setCurrentText(csn);
  }

  if (!config_loaded_) {
    load_from_config();
    config_loaded_ = true;
  }
}
void SettingsDialog::build_ui() {
  auto *vbox = new QVBoxLayout(this);
  tab_widget_ = new QTabWidget(this);

  // --- General Tab ---
  auto *general_tab = new QWidget(tab_widget_);
  auto *general_vbox = new QVBoxLayout(general_tab);

  auto *mg = new QGroupBox("Mic → Camera Mappings", general_tab);
  auto *mgl = new QVBoxLayout(mg);
  mappings_table_ = new QTableWidget(0, 6, mg);
  mappings_table_->setHorizontalHeaderLabels(
      {"Audio Source", "Video Source", "Scene/Camera", "Priority", "Threshold", ""});
  mappings_table_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);
  mappings_table_->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::Stretch);
  mappings_table_->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::Stretch);
  mappings_table_->setColumnWidth(3, 90);
  mappings_table_->setColumnWidth(4, 90);
  mappings_table_->setColumnWidth(5, 30);
  mappings_table_->verticalHeader()->setVisible(false);
  mgl->addWidget(mappings_table_);
  auto *br = new QHBoxLayout();
  add_btn_ = new QPushButton("+ Add", mg);
  remove_btn_ = new QPushButton("− Remove", mg);
  br->addWidget(add_btn_);
  br->addWidget(remove_btn_);
  br->addStretch();
  mgl->addLayout(br);
  general_vbox->addWidget(mg);
  connect(add_btn_, &QPushButton::clicked, this,
          &SettingsDialog::on_add_mapping);
  connect(remove_btn_, &QPushButton::clicked, this,
          &SettingsDialog::on_remove_mapping);
  auto *gg = new QGroupBox("Global Settings", this);
  auto *form = new QFormLayout(gg);
  auto *rr = new QHBoxLayout();
  relaxed_radio_ = new QRadioButton("Relaxed", gg);
  neutral_radio_ = new QRadioButton("Neutral", gg);
  fast_radio_ = new QRadioButton("Fast", gg);
  neutral_radio_->setChecked(true);
  rr->addWidget(relaxed_radio_);
  rr->addWidget(neutral_radio_);
  rr->addWidget(fast_radio_);
  rr->addStretch();
  form->addRow("Switching Speed:", rr);

  auto *mir = new QHBoxLayout();
  mi_off_radio_ = new QRadioButton("Off", gg);
  mi_moderate_radio_ = new QRadioButton("Moderate", gg);
  mi_high_radio_ = new QRadioButton("High", gg);
  mi_moderate_radio_->setChecked(true);
  mir->addWidget(mi_off_radio_);
  mir->addWidget(mi_moderate_radio_);
  mir->addWidget(mi_high_radio_);
  mir->addStretch();
  form->addRow("Motion Influence:", mir);

  general_vbox->addWidget(gg);
  tab_widget_->addTab(general_tab, "General");

  // --- Scene Generator Tab ---
  auto *gen_tab = new QWidget(tab_widget_);
  auto *gen_vbox = new QVBoxLayout(gen_tab);
  
  auto *guide_frame = new QFrame(gen_tab);
  guide_frame->setFrameShape(QFrame::StyledPanel);
  guide_frame->setStyleSheet("QFrame { background-color: rgba(60, 140, 255, 0.1); border-radius: 4px; border: 1px solid rgba(60, 140, 255, 0.3); margin-bottom: 8px; }");
  auto *guide_layout = new QVBoxLayout(guide_frame);
  auto *guide_label = new QLabel(
      "<b>Podcast Setup Workflow:</b><br/>"
      "<ol style='margin-top: 4px; margin-bottom: 0px; padding-left: 20px;'>"
      "<li><b>Format:</b> Select your podcast format below and click <i>Generate</i>.</li>"
      "<li><b>Video:</b> Go to the newly generated <i>Input Scenes</i> in OBS and add your cameras.</li>"
      "<li><b>Audio:</b> Go to the <i>General</i> tab here to map your microphones to the new scenes.</li>"
      "</ol>",
      guide_frame);
  guide_label->setWordWrap(true);
  guide_label->setStyleSheet("QLabel { background: transparent; border: none; }");
  guide_layout->addWidget(guide_label);
  gen_vbox->addWidget(guide_frame);

  auto *gen_form = new QFormLayout();

  gen_format_combo_ = new QComboBox(gen_tab);
  gen_format_combo_->addItem("2-Person Podcast", 0);
  gen_format_combo_->addItem("3-Person Podcast", 1);
  gen_format_combo_->addItem("4-Person Podcast", 2);
  gen_form->addRow("Podcast Format:", gen_format_combo_);

  gen_btn_ = new QPushButton("✨ Generate Podcast Scenes", gen_tab);
  connect(gen_btn_, &QPushButton::clicked, this, &SettingsDialog::on_generate_scenes);

  gen_vbox->addLayout(gen_form);
  gen_vbox->addStretch();
  gen_vbox->addWidget(gen_btn_);
  tab_widget_->addTab(gen_tab, "Podcast Format");

  vbox->addWidget(tab_widget_);
  auto *bb = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
      this);
  connect(bb, &QDialogButtonBox::accepted, this, &SettingsDialog::on_ok);
  connect(bb->button(QDialogButtonBox::Apply), &QPushButton::clicked, this,
          &SettingsDialog::on_apply);
  connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
  vbox->addWidget(bb);
}
void SettingsDialog::add_mapping_row(const CamMapping &m) {
  int row = mappings_table_->rowCount();
  mappings_table_->insertRow(row);
  auto *sc = new QComboBox();
  bool found_audio = false;
  for (auto &s : audio_sources_) {
    sc->addItem(QString::fromStdString(s));
    if (s == m.audio_source) found_audio = true;
  }
  if (!m.audio_source.empty() && !found_audio) {
      sc->addItem(QString::fromStdString(m.audio_source) + " (Missing)");
      sc->setItemData(sc->count() - 1, QColor(Qt::red), Qt::ForegroundRole);
      sc->setCurrentText(QString::fromStdString(m.audio_source) + " (Missing)");
  } else {
      sc->setCurrentText(QString::fromStdString(m.audio_source));
  }
  mappings_table_->setCellWidget(row, 0, sc);

  auto *vc = new QComboBox();
  vc->addItem("— None —");
  bool found_video = false;
  for (auto &s : video_sources_) {
      vc->addItem(QString::fromStdString(s));
      if (s == m.video_source) found_video = true;
  }
  if (!m.video_source.empty() && !found_video) {
      vc->addItem(QString::fromStdString(m.video_source) + " (Missing)");
      vc->setItemData(vc->count() - 1, QColor(Qt::red), Qt::ForegroundRole);
      vc->setCurrentText(QString::fromStdString(m.video_source) + " (Missing)");
  } else {
      vc->setCurrentText(m.video_source.empty() ? "— None —" : QString::fromStdString(m.video_source));
  }
  mappings_table_->setCellWidget(row, 1, vc);

  auto *ss = new QComboBox();
  for (auto &s : scene_names_)
    ss->addItem(QString::fromStdString(s));
  ss->setCurrentText(QString::fromStdString(m.scene_name));
  mappings_table_->setCellWidget(row, 2, ss);
  auto *pc = new QComboBox();
  pc->addItem("Low", (int)Priority::Low);
  pc->addItem("Medium", (int)Priority::Medium);
  pc->addItem("High", (int)Priority::High);
  pc->setCurrentIndex((int)m.priority);
  mappings_table_->setCellWidget(row, 3, pc);
  auto *ts = new QSpinBox();
  ts->setRange(-96, 0);
  ts->setValue((int)m.threshold_dbfs);
  ts->setSuffix(" dB");
  mappings_table_->setCellWidget(row, 4, ts);
  auto *db = new QPushButton("✕");
  db->setFixedWidth(28);
  connect(db, &QPushButton::clicked, this, [this]() {
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;
    for (int r = 0; r < mappings_table_->rowCount(); ++r) {
      if (mappings_table_->cellWidget(r, 5) == btn) {
        mappings_table_->removeRow(r);
        break;
      }
    }
  });
  mappings_table_->setCellWidget(row, 5, db);
}
void SettingsDialog::on_add_mapping() { add_mapping_row({}); }
void SettingsDialog::on_remove_mapping() {
  int r = mappings_table_->currentRow();
  if (r >= 0)
    mappings_table_->removeRow(r);
}
void SettingsDialog::load_from_config() {
  mappings_table_->setRowCount(0);
  for (auto &m : config_->get_mappings())
    add_mapping_row(m);
  switch (config_->get_responsiveness()) {
  case Responsiveness::Relaxed:
    relaxed_radio_->setChecked(true);
    break;
  case Responsiveness::Fast:
    fast_radio_->setChecked(true);
    break;
  default:
    neutral_radio_->setChecked(true);
  }

  switch (config_->get_motion_influence()) {
  case MotionInfluence::Off:
    mi_off_radio_->setChecked(true);
    break;
  case MotionInfluence::High:
    mi_high_radio_->setChecked(true);
    break;
  default:
    mi_moderate_radio_->setChecked(true);
  }
  // Scene Generator Settings
  int format = config_->get_gen_format();
  int f_idx = gen_format_combo_->findData(format);
  if (f_idx >= 0) gen_format_combo_->setCurrentIndex(f_idx);
}
void SettingsDialog::save_to_config() {
  std::vector<CamMapping> mappings;
  for (int r = 0; r < mappings_table_->rowCount(); ++r) {
    CamMapping m;
    auto *sc = qobject_cast<QComboBox *>(mappings_table_->cellWidget(r, 0));
    auto *vc = qobject_cast<QComboBox *>(mappings_table_->cellWidget(r, 1));
    auto *ss = qobject_cast<QComboBox *>(mappings_table_->cellWidget(r, 2));
    auto *pc = qobject_cast<QComboBox *>(mappings_table_->cellWidget(r, 3));
    auto *ts = qobject_cast<QSpinBox *>(mappings_table_->cellWidget(r, 4));
    if (!sc || !vc || !ss || !pc || !ts)
      continue;
    std::string audio_str = sc->currentText().toStdString();
    if (audio_str.size() > 10 && audio_str.substr(audio_str.size() - 10) == " (Missing)") {
        audio_str = audio_str.substr(0, audio_str.size() - 10);
    }
    m.audio_source = audio_str;

    std::string video_str = vc->currentText() == "— None —" ? "" : vc->currentText().toStdString();
    if (video_str.size() > 10 && video_str.substr(video_str.size() - 10) == " (Missing)") {
        video_str = video_str.substr(0, video_str.size() - 10);
    }
    m.video_source = video_str;
    m.scene_name = ss->currentText().toStdString();
    m.priority = (Priority)pc->currentData().toInt();
    m.threshold_dbfs = (float)ts->value();
    mappings.push_back(m);
  }
  config_->set_mappings(mappings);
  Responsiveness r = Responsiveness::Neutral;
  if (relaxed_radio_->isChecked())
    r = Responsiveness::Relaxed;
  if (fast_radio_->isChecked())
    r = Responsiveness::Fast;
  config_->set_responsiveness(r);

  MotionInfluence mi = MotionInfluence::Moderate;
  if (mi_off_radio_->isChecked())
    mi = MotionInfluence::Off;
  if (mi_high_radio_->isChecked())
    mi = MotionInfluence::High;
  config_->set_motion_influence(mi);

  config_->set_gen_format(gen_format_combo_->currentData().toInt());

  config_->save();
  emit settings_applied(*config_);
}
void SettingsDialog::on_apply() { save_to_config(); }
void SettingsDialog::on_ok() {
  save_to_config();
  accept();
}

#include "../scene-generator.h"
#include <QMessageBox>
#include <algorithm>
#include <QInputDialog>

void SettingsDialog::on_generate_scenes() {
  SceneGenSettings sgs;
  sgs.format = (PodcastFormat)gen_format_combo_->currentData().toInt();
  sgs.audio_sources = audio_sources_;

  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(this, "Generate Scenes",
                                "Are you sure you want to generate new scenes?\n\n"
                                "This will delete all previously generated scenes and their layouts.",
                                QMessageBox::Yes | QMessageBox::No);
  if (reply != QMessageBox::Yes) {
      return;
  }

  if (SceneGenerator::generate(sgs)) {
    // Add newly generated scenes to cached list so they can be selected
    const char *new_scenes[] = {
        "🟢 Starting Soon", "👤 Person 1", "👤 Person 2", "👤 Person 3", "👤 Person 4",
        "👥 Split Screen", "🔴 Thank You", "--- INPUTS ---",
        "📺 Inputs / Person 1", "📺 Inputs / Person 2", "📺 Inputs / Person 3", "📺 Inputs / Person 4",
        "\U0001F399\uFE0F Live Audio Mix"
    };
    for (const char *ns : new_scenes) {
        if (std::find(scene_names_.begin(), scene_names_.end(), ns) == scene_names_.end()) {
            scene_names_.push_back(ns);
        }
    }
    
    // Also add them to video_sources_ so they can be selected in the video mapping!
    for (const char *ns : new_scenes) {
        if (std::find(video_sources_.begin(), video_sources_.end(), ns) == video_sources_.end()) {
            video_sources_.push_back(ns);
        }
    }
    
    mappings_table_->setRowCount(0);
    
    auto find_audio = [&](const std::string& v) -> std::string {
        if (!audio_sources_.empty()) {
            return audio_sources_[0]; 
        }
        return "";
    };

    int num_people = 2;
    if (sgs.format == PodcastFormat::ThreePerson) num_people = 3;
    if (sgs.format == PodcastFormat::FourPerson) num_people = 4;
    
    for (int i = 1; i <= num_people; ++i) {
        CamMapping m;
        m.video_source = "📺 Inputs / Person " + std::to_string(i);
        m.audio_source = audio_sources_.size() >= (size_t)i ? audio_sources_[i-1] : find_audio(m.video_source);
        m.scene_name = "👤 Person " + std::to_string(i);
        m.priority = Priority::Medium;
        add_mapping_row(m);
    }
    
    QMessageBox::information(this, "Scenes Generated", "Successfully generated podcast scenes and auto-populated mappings!");
    save_to_config();
  }
}
