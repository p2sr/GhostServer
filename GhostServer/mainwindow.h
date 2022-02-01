#pragma once

#include <QWidget>
#include "ui_mainwindow.h"
#include "networkmanager.h"

class MainWindow : public QWidget
{
	Q_OBJECT


private:

	NetworkManager network;
    bool isRunning;

public:
	MainWindow(QWidget *parent = Q_NULLPTR);
	~MainWindow();

private:
	Ui::MainWindow ui;

	void StartServer();
    void StopServer();
    void ResetServer();

public slots:
    void AddEventLog(QString log) {ui.textBrowser->append(log);}
    void StartCountdown();


};
