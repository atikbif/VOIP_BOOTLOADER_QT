#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "loadconfig.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QSettings>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->progressBar->setValue(0);
    LoadConfig conf(ui->lineEditIP->text(),ui->spinBoxGroup->value(),ui->spinBoxPoint->value());

    QCoreApplication::setOrganizationName("Modern Techikal Solutions");
    QCoreApplication::setOrganizationDomain("str-sib.ru");
    QCoreApplication::setApplicationName("CAN VOIP bootloader");

    QSettings settings;

    QString ip = settings.value("ip", "192.168.1.2").toString();
    ui->lineEditIP->setText("IP:" + ip);
    openPath = settings.value("openPath", "").toString();



    boot = new BootloaderController(conf);
    connect(boot,&BootloaderController::percentUpdate,[=](int value){ui->progressBar->setValue(value);});
    connect(boot,&BootloaderController::loadError,[=](const QString &message){ui->statusbar->showMessage(message,0);});
    connect(boot,&BootloaderController::info,[=](const QString &message){ui->statusbar->showMessage(message,0);});
    connect(boot,&BootloaderController::loadComplete,[=](){ui->statusbar->showMessage("Загрузка завершена",0);});

    ui->pushButtonStart->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButtonStart_released()
{
    QSettings settings;
    QString fName;
    QString loadPath = settings.value("loadPath","").toString();
    if(firstBoot) {
        auto reply = QMessageBox::question(this, "Загрузка ПО", "Загрузка приведёт к стиранию текущей программы.\nПродолжить?",
                                        QMessageBox::Yes|QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
    }
        fName = QFileDialog::getOpenFileName(this, "Open File",
                                                        loadPath,
                                                        "Файл прошивки (*.binary)");
    firstBoot = false;
    if(!fName.isEmpty()) {
        qDebug() << fName;
        LoadConfig conf(ui->lineEditIP->text(),ui->spinBoxGroup->value(),ui->spinBoxPoint->value());
        conf.setFileName(fName);
        settings.setValue("ip",conf.getIP());
        QFileInfo fInfo(fName);
        settings.setValue("loadPath",fInfo.absolutePath());
        boot->updateConfig(conf);
        emit boot->load();
    }
}
