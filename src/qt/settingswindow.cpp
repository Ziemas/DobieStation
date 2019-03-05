#include <QVBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QDialog>
#include <QWidget>
#include <QGroupBox>
#include <QRadioButton>

#include "settingswindow.hpp"
#include "settings.hpp"

GeneralTab::GeneralTab(QWidget* parent)
    : QWidget(parent)
{
    QPushButton* browse_button = new QPushButton(tr("Browse"), this);
    connect(browse_button, &QAbstractButton::clicked, this,
        &GeneralTab::browse_for_bios);

    bios_reader = BiosReader(Settings::get_bios_path());
    bios_info = new QLabel(bios_reader.to_string(), this);

    QGridLayout* bios_layout = new QGridLayout(this);
    bios_layout->addWidget(bios_info, 0, 0);
    bios_layout->addWidget(browse_button, 0, 3);

    bios_layout->setColumnStretch(1, 1);
    bios_layout->setAlignment(Qt::AlignTop);

    QGroupBox* bios_groupbox = new QGroupBox(tr("Bios"), this);
    bios_groupbox->setLayout(bios_layout);

    QRadioButton* jit_checkbox = new QRadioButton(tr("JIT"), this);
    QRadioButton* interpreter_checkbox = new QRadioButton(tr("Interpreter"), this);

    vu1_jit = Settings::get_vu1_jit_enabled();
    jit_checkbox->setChecked(vu1_jit);
    interpreter_checkbox->setChecked(!vu1_jit);

    connect(jit_checkbox, &QRadioButton::clicked, this, [=] (){
        this->vu1_jit = true;
    });

    connect(interpreter_checkbox, &QRadioButton::clicked, this, [=] (){
        this->vu1_jit = false;
    });

    QVBoxLayout* vu1_layout = new QVBoxLayout(this);
    vu1_layout->addWidget(jit_checkbox);
    vu1_layout->addWidget(interpreter_checkbox);

    QGroupBox* vu1_groupbox = new QGroupBox(tr("VU1"), this);
    vu1_groupbox->setLayout(vu1_layout);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(bios_groupbox);
    layout->addWidget(vu1_groupbox);

    setLayout(layout);

    setMinimumWidth(400);
}

void GeneralTab::browse_for_bios()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("Open Bios"), Settings::get_last_used_path(),
        tr("Bios File (*.bin)")
    );

    Settings::set_last_used_path(path);

    if (!path.isEmpty())
    {
        bios_reader = BiosReader(path);

        bios_info->setText(bios_reader.to_string());
    }
}

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    general_tab = new GeneralTab(this);

    QTabWidget* tab_widget = new QTabWidget(this);
    tab_widget->addTab(general_tab, tr("General"));

    QDialogButtonBox* close_button =
        new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(close_button, &QDialogButtonBox::rejected, this,
        &SettingsWindow::save_and_reject);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(tab_widget);
    layout->addWidget(close_button);

    setLayout(layout);
}

void SettingsWindow::save_and_reject()
{
    Settings::set_bios_path(general_tab->bios_reader.path());
    Settings::set_vu1_jit_enabled(general_tab->vu1_jit);
    Settings::save();

    reject();
}