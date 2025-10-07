#pragma once

#include <QWidget>

#include "commands.h"
#include "ui_mainwindow.h"
#include "networkmanager.h"

struct CountdownPreset {
    QString name;
    QString preCommands;
    QString postCommands;
};

class MainWindow : public QWidget
{
	Q_OBJECT


private:

    NetworkManager *network;
    bool isRunning;

public:
	MainWindow(QWidget *parent = Q_NULLPTR);
	~MainWindow();

private:
	Ui::MainWindow ui;

	void StartServer();
    void StopServer();
    void ResetServer();
    void SubmitCommand();
    void OnPresetChanged(int index);

public slots:
    void AddEventLog(QString log) {ui.textBrowser->append(log);}
    void StartCountdown();


};
