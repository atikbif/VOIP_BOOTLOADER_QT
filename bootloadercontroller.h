#ifndef BOOTLOADERCONTROLLER_H
#define BOOTLOADERCONTROLLER_H

#include <QDialog>
#include <QThread>
#include <QLabel>
#include <QProgressBar>
#include "bootloader.h"
#include "loadconfig.h"

class BootloaderController : public QObject
{
    QThread loaderThread;
    QProgressBar *bar;
    QLabel *message;
    QString ipAddress;
    BootLoader *loader;
    Q_OBJECT
public:
    explicit BootloaderController(const LoadConfig &conf, QObject *parent = nullptr);
    void updateConfig(const LoadConfig &conf);
    ~BootloaderController();

signals:
    void load();
    void loadComplete();
    void loadError(const QString &message);
    void info(const QString &message);
    void percentUpdate(int value);
};

#endif // BOOTLOADERCONTROLLER_H
