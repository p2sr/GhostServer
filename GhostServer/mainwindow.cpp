#include "mainwindow.h"

#include "commands.h"
#include "networkmanager.h"

#include <qpushbutton.h>
#include <qtextdocument.h>
#include <QTextBlock>

#include <qdebug.h>

std::vector<CountdownPreset> countdownPresets = {
    {
        "None",
        "",
        ""
    },
    {
        "Fullgame",
        "ghost_sync 1\nghost_sync_countdown 3\nsvar_set sp_use_save 2\nghost_leaderboard_mode 1\nghost_leaderboard_reset\nsar_on_load conds map=sp_a1_wakeup \"ghost_sync 0\" map=sp_a2_intro \"ghost_sync 1\"",
        "sar_speedrun_skip_cutscenes 1\nsar_speedrun_offset 18980\nsar_speedrun_reset\nstop\nsv_allow_mobile_portals 0\nload vault"
    },
    {
        "Speedrun Mod",
        "ghost_sync 1\nghost_sync_countdown 3\nsvar_set sp_use_save 2\nghost_leaderboard_mode 1\nghost_leaderboard_reset",
        "sar_speedrun_offset 0\nsar_speedrun_reset\nstop\nsv_allow_mobile_portals 0\nmap sp_a1_intro1"
    },
    {
        "Portal Stories: Mel",
        "ghost_sync 1\nghost_sync_countdown 3\nghost_leaderboard_mode 1\nghost_leaderboard_reset\nsar_ent_slot_serial 838 16301",
        "sar_speedrun_reset\nmap st_a1_tramride"
    }
};

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    this->isRunning = false;
    ui.resetButton->setVisible(false);
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    ui.textBrowser->setFont(font);

    network = new NetworkManager("ghost_log");

    connect(network, &NetworkManager::OnNewEvent, this, &MainWindow::AddEventLog);
    connect(network, &NetworkManager::UIEvent, this, &MainWindow::UIEvent);
    connect(ui.serverButton, &QPushButton::clicked, this, &MainWindow::StartServer);
    connect(ui.startCountdown, &QPushButton::clicked, this, &MainWindow::StartCountdown);
    connect(ui.resetButton, &QPushButton::clicked, this, &MainWindow::ResetServer);
    connect(ui.submitCommandButton, &QPushButton::clicked, this, &MainWindow::SubmitCommand);
    connect(ui.commandInput, &QLineEdit::returnPressed, this, &MainWindow::SubmitCommand);
    connect(ui.presetDropdown, &QComboBox::currentIndexChanged, this, &MainWindow::OnPresetChanged);

    for (const auto& preset : countdownPresets) {
        ui.presetDropdown->addItem(preset.name);
    }
    ui.presetDropdown->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    if (this->isRunning) this->network->StopServer();
    delete this->network;
}

void MainWindow::StartServer()
{
    if (!this->network->StartServer(ui.port->value())) {
        this->AddEventLog("Server didn't start! Please check settings!");
        return;
    }

    this->isRunning = true;
    ui.serverButton->setText("Stop server");
    ui.resetButton->setVisible(true);
    disconnect(ui.serverButton, &QPushButton::clicked, this, &MainWindow::StartServer);
    connect(ui.serverButton, &QPushButton::clicked, this, &MainWindow::StopServer);
}

void MainWindow::StopServer()
{
    this->network->StopServer();

    this->isRunning = false;
    ui.serverButton->setText("Start server");
    ui.resetButton->setVisible(false);

    disconnect(ui.serverButton, &QPushButton::clicked, this, &MainWindow::StopServer);
    connect(ui.serverButton, &QPushButton::clicked, this, &MainWindow::StartServer);
}

void MainWindow::ResetServer()
{
    ui.resetButton->setVisible(false);
}

void MainWindow::StartCountdown()
{
    QTextDocument* pre_doc = ui.preCommandList->document();
    QString pre_cmds = pre_doc->toPlainText().split(QString("\n")).join(QString("; "));


    QTextDocument* post_doc = ui.postCommandList->document();
    QString post_cmds = post_doc->toPlainText().split(QString("\n")).join(QString("; "));

    int duration = ui.duration->value();
    this->network->StartCountdown(pre_cmds.toStdString(), post_cmds.toStdString(), duration);
}

void MainWindow::SubmitCommand()
{
    QString command = ui.commandInput->text();

    handle_cmd(this->network, command.toStdString().data());

    if (g_should_stop) {
        MainWindow::StopServer();
        g_should_stop = 0;
    }

    ui.commandInput->clear();
}

void MainWindow::OnPresetChanged(int index)
{
    if (index < 0 || index >= static_cast<int>(countdownPresets.size())) return;

    const CountdownPreset& preset = countdownPresets[index];
    ui.preCommandList->setPlainText(preset.preCommands);
    ui.postCommandList->setPlainText(preset.postCommands);
}

void MainWindow::UIEvent(std::string event)
{
}
