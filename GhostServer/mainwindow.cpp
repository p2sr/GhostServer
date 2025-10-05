#include "mainwindow.h"

#include "networkmanager.h"

#include <qpushbutton.h>
#include <qtextdocument.h>
#include <QTextBlock>

#include <qdebug.h>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    this->isRunning = false;
    ui.resetButton->setVisible(false);

    connect(&network, &NetworkManager::OnNewEvent, this, &MainWindow::AddEventLog);
    connect(ui.serverButton, &QPushButton::clicked, this, &MainWindow::StartServer);
    connect(ui.startCountdown, &QPushButton::clicked, this, &MainWindow::StartCountdown);
    connect(ui.resetButton, &QPushButton::clicked, this, &MainWindow::ResetServer);
}

MainWindow::~MainWindow()
{
    if (this->isRunning) this->network.StopServer();
}

void MainWindow::StartServer()
{
    if (!this->network.StartServer(ui.port->value())) {
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
    this->network.StopServer();

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
    QString pre_cmds = "";
    for (int i = 0; i < pre_doc->lineCount(); ++i) {
        QTextBlock tb = pre_doc->findBlockByLineNumber(i);
        pre_cmds += tb.text();
        if (i < pre_doc->lineCount() - 1) {
            pre_cmds += "; ";
        }
    }


    QTextDocument* post_doc = ui.postCommandList->document();
    QString post_cmds = "";
    for (int i = 0; i < post_doc->lineCount(); ++i) {
        QTextBlock tb = post_doc->findBlockByLineNumber(i);
        post_cmds += tb.text(); 
        if (i < post_doc->lineCount() - 1) {
            post_cmds += "; ";
        }
    }

    int duration = ui.duration->value();
    this->network.StartCountdown(pre_cmds.toStdString(), post_cmds.toStdString(), duration);
}
