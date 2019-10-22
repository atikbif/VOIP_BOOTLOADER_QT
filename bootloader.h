#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QObject>
#include <QMutex>
#include <QString>
#include "loadconfig.h"
#include <QDebug>

class BootLoader : public QObject
{
    QMutex mutex;
    bool stopFlag;
    QString ipAddress;
    LoadConfig conf;
    static const int TRY_LIM = 10;

    QByteArray createErasePageRequest(int pageNum);
    QByteArray createWriteFlashRequest(quint32 addr, quint16 cnt, QByteArray::ConstIterator it);
    QByteArray createJumpToAppRequest();
    QByteArray createSetBootloaderStateRequest();
    //QByteArray createReadTypeRequest();
    bool checkCRCAnswer(char *data, int length);

    Q_OBJECT
public:
    explicit BootLoader(const LoadConfig &conf, QObject *parent = nullptr);
    void updateConfig(const LoadConfig &conf) {this->conf=conf;qDebug() << this->conf.getFileName();}

signals:
    void loadComplete();
    void loadError(const QString &message);
    void info(const QString &message);
    void percentUpdate(int value);
public slots:
    void load();
    void stopLoad();
};

#endif // BOOTLOADER_H
