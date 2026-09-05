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
    const std::vector<std::string> &scene_names) {
  audio_sources_ = audio_sources;
  scene_names_ = scene_names;
  for (int row = 0; row < mappings_table_->rowCount(); ++row) {
    auto *sc = qobject_cast<QComboBox *>(mappings_table_->cellWidget(row, 0));
    auto *ss = qobject_cast<QComboBox *>(mappings_table_->cellWidget(row, 1));
    if (!sc || !ss)
      continue;
    QString cs = sc->currentText(), csn = ss->currentText();
    sc->clear();
    ss->clear();
    for (auto &s : audio_sources_)
      sc->addItem(QString::fromStdString(s));
    for (auto &s : scene_names_)
      ss->addItem(QString::fromStdString(s));
    sc->setCurrentText(cs);
    ss->setCurrentText(csn);
  }
  QString cf = fallback_combo_->currentText();
  fallback_combo_->clear();
  fallback_combo_->addItem("— None —", "");
  for (auto &s : scene_names_) {
    fallback_combo_->addItem(QString::fromStdString(s),
                             QString::fromStdString(s));
  }
  fallback_combo_->setCurrentText(cf);

  QString h1 = gen_host1_combo_->currentText();
  QString g1 = gen_guest1_combo_->currentText();
  QString g2 = gen_guest2_combo_->currentText();
  gen_host1_combo_->clear();
  gen_guest1_combo_->clear();
  gen_guest2_combo_->clear();
  for (auto &s : scene_names_) {
    QString qstr = QString::fromStdString(s);
    gen_host1_combo_->addItem(qstr, qstr);
    gen_guest1_combo_->addItem(qstr, qstr);
    gen_guest2_combo_->addItem(qstr, qstr);
  }
  gen_host1_combo_->setCurrentText(h1);
  gen_guest1_combo_->setCurrentText(g1);
  gen_guest2_combo_->setCurrentText(g2);
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
  mappings_table_ = new QTableWidget(0, 5, mg);
  mappings_table_->setHorizontalHeaderLabels(
      {"Audio Source", "Scene/Camera", "Priority", "Threshold", ""});
  mappings_table_->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);
  mappings_table_->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::Stretch);
  mappings_table_->setColumnWidth(2, 90);
  mappings_table_->setColumnWidth(3, 110);
  mappings_table_->setColumnWidth(4, 30);
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
  form->addRow("Responsiveness:", rr);
  hold_time_spin_ = new QSpinBox(gg);
  hold_time_spin_->setRange(100, 10000);
  hold_time_spin_->setValue(800);
  hold_time_spin_->setSuffix(" ms");
  form->addRow("Hold Time:", hold_time_spin_);
  fallback_combo_ = new QComboBox(gg);
  fallback_combo_->addItem("— None —", "");
  form->addRow("Fallback Scene:", fallback_combo_);
  auto *tr = new QHBoxLayout();
  fade_check_ = new QCheckBox("Fade", gg);
  fade_duration_spin_ = new QSpinBox(gg);
  fade_duration_spin_->setRange(100, 3000);
  fade_duration_spin_->setValue(300);
  fade_duration_spin_->setSuffix(" ms");
  fade_duration_spin_->setEnabled(false);
  connect(fade_check_, &QCheckBox::toggled, fade_duration_spin_,
          &QSpinBox::setEnabled);
  tr->addWidget(fade_check_);
  tr->addWidget(fade_duration_spin_);
  tr->addStretch();
  form->addRow("Transition:", tr);
  general_vbox->addWidget(gg);
  tab_widget_->addTab(general_tab, "General");

  // --- Scene Generator Tab ---
  auto *gen_tab = new QWidget(tab_widget_);
  auto *gen_vbox = new QVBoxLayout(gen_tab);
  auto *gen_form = new QFormLayout();

  gen_format_combo_ = new QComboBox(gen_tab);
  gen_format_combo_->addItem("1-on-1 Interview", 0);
  gen_format_combo_->addItem("Power Dynamic (3-person)", 1);
  gen_form->addRow("Podcast Format:", gen_format_combo_);

  gen_host1_combo_ = new QComboBox(gen_tab);
  gen_guest1_combo_ = new QComboBox(gen_tab);
  gen_guest2_combo_ = new QComboBox(gen_tab);
  gen_form->addRow("Host 1 Video Source:", gen_host1_combo_);
  gen_form->addRow("Guest 1 Video Source:", gen_guest1_combo_);
  gen_form->addRow("Guest 2 Video Source:", gen_guest2_combo_);

  auto *gen_btn = new QPushButton("✨ Generate Podcast Scenes", gen_tab);
  connect(gen_btn, &QPushButton::clicked, this, &SettingsDialog::on_generate_scenes);

  gen_vbox->addLayout(gen_form);
  gen_vbox->addStretch();
  gen_vbox->addWidget(gen_btn);
  tab_widget_->addTab(gen_tab, "Scene Generator");

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
  for (auto &s : audio_sources_)
    sc->addItem(QString::fromStdString(s));
  sc->setCurrentText(QString::fromStdString(m.audio_source));
  mappings_table_->setCellWidget(row, 0, sc);
  auto *ss = new QComboBox();
  for (auto &s : scene_names_)
    ss->addItem(QString::fromStdString(s));
  ss->setCurrentText(QString::fromStdString(m.scene_name));
  mappings_table_->setCellWidget(row, 1, ss);
  auto *pc = new QComboBox();
  pc->addItem("Low", (int)Priority::Low);
  pc->addItem("Medium", (int)Priority::Medium);
  pc->addItem("High", (int)Priority::High);
  pc->setCurrentIndex((int)m.priority);
  mappings_table_->setCellWidget(row, 2, pc);
  auto *ts = new QSpinBox();
  ts->setRange(-96, 0);
  ts->setValue((int)m.threshold_dbfs);
  ts->setSuffix(" dB");
  mappings_table_->setCellWidget(row, 3, ts);
  auto *db = new QPushButton("✕");
  db->setFixedWidth(28);
  connect(db, &QPushButton::clicked, this, [this]() {
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;
    for (int r = 0; r < mappings_table_->rowCount(); ++r) {
      if (mappings_table_->cellWidget(r, 4) == btn) {
        mappings_table_->removeRow(r);
        break;
      }
    }
  });
  mappings_table_->setCellWidget(row, 4, db);
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
  hold_time_spin_->setValue(config_->get_hold_time_ms());
  QString fb = QString::fromStdString(config_->get_fallback_scene());
  int idx = fallback_combo_->findData(fb);
  if (idx >= 0)
    fallback_combo_->setCurrentIndex(idx);

  fade_check_->setChecked(config_->get_transition_fade());
  fade_duration_spin_->setValue(config_->get_fade_duration_ms());
}
void SettingsDialog::save_to_config() {
  std::vector<CamMapping> mappings;
  for (int r = 0; r < mappings_table_->rowCount(); ++r) {
    CamMapping m;
    auto *sc = qobject_cast<QComboBox *>(mappings_table_->cellWidget(r, 0));
    auto *ss = qobject_cast<QComboBox *>(mappings_table_->cellWidget(r, 1));
    auto *pc = qobject_cast<QComboBox *>(mappings_table_->cellWidget(r, 2));
    auto *ts = qobject_cast<QSpinBox *>(mappings_table_->cellWidget(r, 3));
    if (!sc || !ss || !pc || !ts)
      continue;
    m.audio_source = sc->currentText().toStdString();
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
  config_->set_hold_time_ms(hold_time_spin_->value());
  config_->set_fallback_scene(
      fallback_combo_->currentData().toString().toStdString());
  config_->set_transition_fade(fade_check_->isChecked());
  config_->set_fade_duration_ms(fade_duration_spin_->value());
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

void SettingsDialog::on_generate_scenes() {
  SceneGenSettings sgs;
  sgs.format = (PodcastFormat)gen_format_combo_->currentData().toInt();
  sgs.host1_source = gen_host1_combo_->currentText().toStdString();
  sgs.guest1_source = gen_guest1_combo_->currentText().toStdString();
  sgs.guest2_source = gen_guest2_combo_->currentText().toStdString();
  sgs.audio_sources = audio_sources_; // For now, put all known audio sources in the live mix

  if (SceneGenerator::generate(sgs)) {
    QMessageBox::information(this, "Scenes Generated", "Successfully generated podcast scenes!");
  }
}

