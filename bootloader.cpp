#include "bootloader.h"
#include <QUdpSocket>
#include "checksum.h"
#include <QFile>
#include <QByteArray>
#include <QThread>

QByteArray BootLoader::createErasePageRequest(int pageNum)
{
    QByteArray res;
    static quint16 id = 0;
    res.append(static_cast<char>(id>>8));
    res.append(static_cast<char>(id&0xFF));
    res.append(0x11);
    res.append(static_cast<char>(pageNum));
    res.append(static_cast<char>(conf.getGroup()));
    res.append(static_cast<char>(conf.getPoint()));
    int crc = CheckSum::getCRC16(res);
    res.append(static_cast<char>(crc & 0xFF));
    res.append(static_cast<char>(crc >> 8));
    id++;
    return res;
}

QByteArray BootLoader::createWriteFlashRequest(quint32 addr, quint16 cnt, QByteArray::ConstIterator it)
{
    QByteArray res;
    static quint16 id = 0;
    res.append(static_cast<char>(id>>8));
    res.append(static_cast<char>(id&0xFF));
    res.append(0x10);
    res.append(static_cast<char>((addr>>24)&0xFF));
    res.append(static_cast<char>((addr>>16)&0xFF));
    res.append(static_cast<char>((addr>>8)&0xFF));
    res.append(static_cast<char>(addr&0xFF));

    res.append(static_cast<char>(conf.getGroup()));
    res.append(static_cast<char>(conf.getPoint()));
    res.append(static_cast<char>(id&0xFF));
    res.append(static_cast<char>(cnt));
    for (quint16 i=0;i<cnt;i++) {
        res.append(*it);
        it++;
    }
    int crc = CheckSum::getCRC16(res);
    res.append(static_cast<char>(crc & 0xFF));
    res.append(static_cast<char>(crc >> 8));
    id++;
    return res;
}

QByteArray BootLoader::createSetBootloaderStateRequest()
{
    QByteArray res;
    static quint16 id = 0;
    res.append(static_cast<char>(id>>8));
    res.append(static_cast<char>(id&0xFF));
    res.append(0x13);
    res.append(static_cast<char>(conf.getGroup()));
    res.append(static_cast<char>(conf.getPoint()));
    int crc = CheckSum::getCRC16(res);
    res.append(static_cast<char>(crc & 0xFF));
    res.append(static_cast<char>(crc >> 8));
    id++;
    return res;
}

QByteArray BootLoader::createJumpToAppRequest()
{
    QByteArray res;
    static quint16 id = 0;
    res.append(static_cast<char>(id>>8));
    res.append(static_cast<char>(id&0xFF));
    res.append(0x14);
    res.append(static_cast<char>(conf.getGroup()));
    res.append(static_cast<char>(conf.getPoint()));
    int crc = CheckSum::getCRC16(res);
    res.append(static_cast<char>(crc & 0xFF));
    res.append(static_cast<char>(crc >> 8));
    id++;
    return res;
}

bool BootLoader::checkCRCAnswer(char *data, int length)
{
    if(CheckSum::getCRC16(data,length)==0) {
        return true;
    }
    return false;
}

BootLoader::BootLoader(const LoadConfig &conf, QObject *parent) : QObject(parent),conf(conf)
{
    stopFlag = false;
}

void BootLoader::load()
{
    int try_num = 0;
    double percent = 0;
    bool exit = false;
    const int max_packet_length = 56;

    double percWriteStep = 1;


    QUdpSocket udp;
    char receiveBuf[1024];
    udp.connectToHost(QHostAddress(conf.getIP()),12146);
    udp.open(QIODevice::ReadWrite);
    udp.readAll();

    qDebug() << conf.getIP();
    quint32 addr = 0;

    QFile binFile(conf.getFileName());
    if(binFile.open(QIODevice::ReadOnly)) {
        qDebug() << "FILE OPENED";
        QByteArray data = binFile.readAll();
        while(data.count()%4) data.append(char(0xFF)); // размер должен быть кратным 4 байтам

        const int pageSize = 2048;
        int pageCnt = data.length()/pageSize;
        if(data.length() % pageSize) pageCnt++;

        percWriteStep = 100.0/pageCnt;

        pageCnt++; // first page contains crc and length


        emit info("Переход в режим загрузчика ...");
        try_num=0;
        while(try_num<TRY_LIM) {
            mutex.lock();
            exit = stopFlag;
            mutex.unlock();
            if(exit) return;
            udp.readAll();
            auto request = createSetBootloaderStateRequest();
            udp.write(request);
            if(udp.waitForReadyRead(100)) {
                int cnt = 0;
                while(udp.hasPendingDatagrams()) {
                    cnt = static_cast<int>(udp.readDatagram(receiveBuf,sizeof(receiveBuf)));
                }
                if(cnt>0) {
                    if(checkCRCAnswer(receiveBuf,cnt)) {
                        break;
                    }
                    else try_num++;
                }
            }else try_num++;
            QThread::msleep(10);
        }
        if(try_num==TRY_LIM) {
            emit loadError("Устройство не отвечает");
            udp.close();
            return;
        }

        emit info("Стирание Flash ...");
        emit percentUpdate(0);

        for(int i=0;i<pageCnt;i++) {
            try_num=0;
            while(try_num<TRY_LIM) {
                mutex.lock();
                exit = stopFlag;
                mutex.unlock();
                if(exit) return;
                udp.readAll();
                auto request = createErasePageRequest(i);
                udp.write(request);
                if(udp.waitForReadyRead(100)) {
                    int cnt = 0;
                    while(udp.hasPendingDatagrams()) {
                        cnt = static_cast<int>(udp.readDatagram(receiveBuf,sizeof(receiveBuf)));
                    }
                    if(cnt>0) {
                        if(checkCRCAnswer(receiveBuf,cnt)) {
                            percent += percWriteStep;
                            emit percentUpdate(static_cast<int>(percent));
                            break;
                        }
                        else try_num++;
                    }
                }else try_num++;
                QThread::msleep(10);
            }
            if(try_num==TRY_LIM) {
                emit loadError("Устройство не отвечает");
                udp.close();
                return;
            }
        }

        emit info("Загрузка ...");
        emit percentUpdate(0);
        auto endIt = data.constEnd();
        auto curIt = data.constBegin();
        quint32 p_cnt = 0;
        auto dist=std::distance(curIt,endIt);
        percWriteStep = 100.0/(static_cast<double>(dist)/max_packet_length);
        percent = 0;

        // write project info in the first page

        addr = 0;
        QByteArray prInfo;
        quint16 crc = static_cast<quint16>(CheckSum::getCRC16(data));
        prInfo.append(static_cast<char>(crc>>8));
        prInfo.append(static_cast<char>(crc&0xFF));
        prInfo.append('\0');
        prInfo.append('\0');
        quint32 prLength = static_cast<quint32>(data.length());
        prInfo.append(static_cast<char>((prLength>>24)&0xFF));
        prInfo.append(static_cast<char>((prLength>>16)&0xFF));
        prInfo.append(static_cast<char>((prLength>>8)&0xFF));
        prInfo.append(static_cast<char>(prLength&0xFF));

        try_num=0;
        while(try_num<TRY_LIM) {
            mutex.lock();
            exit = stopFlag;
            mutex.unlock();
            if(exit) return;
            udp.readAll();
            auto request = createWriteFlashRequest(addr,static_cast<quint16>(prInfo.length()),prInfo.cbegin());
            udp.write(request);
            if(udp.waitForReadyRead(100)) {
                int cnt = 0;
                while(udp.hasPendingDatagrams()) {
                    cnt = static_cast<int>(udp.readDatagram(receiveBuf,sizeof(receiveBuf)));
                }
                if(cnt>0) {
                    if(checkCRCAnswer(receiveBuf,cnt)) {
                        break;
                    }
                    else try_num++;
                }
            }else try_num++;
            QThread::msleep(10);
        }
        if(try_num==TRY_LIM) {
            emit loadError("Устройство не отвечает");
            udp.close();
            return;
        }

        addr = pageSize;
        int length = 0;
        while(dist!=0) {
            p_cnt++;
            if(dist<56) length=dist;else length=56;
            try_num=0;
            while(try_num<TRY_LIM) {
                mutex.lock();
                exit = stopFlag;
                mutex.unlock();
                if(exit) return;
                udp.readAll();
                auto request = createWriteFlashRequest(addr,static_cast<quint16>(length),curIt);
                udp.write(request);
                if(udp.waitForReadyRead(100)) {
                    int cnt = 0;
                    while(udp.hasPendingDatagrams()) {
                        cnt = static_cast<int>(udp.readDatagram(receiveBuf,sizeof(receiveBuf)));
                    }
                    if(cnt>0) {
                        if(checkCRCAnswer(receiveBuf,cnt)) {
                            percent += percWriteStep;
                            emit percentUpdate(static_cast<int>(percent));
                            break;
                        }
                        else try_num++;
                    }
                }else try_num++;
                QThread::msleep(10);
            }
            if(try_num==TRY_LIM) {
                emit loadError("Устройство не отвечает");
                udp.close();
                return;
            }
            curIt+=length;addr+=static_cast<unsigned int>(length);
            dist=std::distance(curIt,endIt);
        }
        //qDebug() << "P_CNT" << p_cnt << data.length();


        emit info("Запуск программы ...");
        try_num=0;
        while(try_num<TRY_LIM) {
            mutex.lock();
            exit = stopFlag;
            mutex.unlock();
            if(exit) return;
            udp.readAll();
            auto request = createJumpToAppRequest();
            udp.write(request);
            if(udp.waitForReadyRead(100)) {
                int cnt = 0;
                while(udp.hasPendingDatagrams()) {
                    cnt = static_cast<int>(udp.readDatagram(receiveBuf,sizeof(receiveBuf)));
                }
                if(cnt>0) {
                    if(checkCRCAnswer(receiveBuf,cnt)) {
                        break;
                    }
                    else try_num++;
                }
            }else try_num++;
            QThread::msleep(10);
        }
        if(try_num==TRY_LIM) {
            emit loadError("Устройство не отвечает");
            udp.close();
            return;
        }
        emit info("Загрузка завершена");
    }else {
        emit loadError("Ошибка открытия файла" + conf.getFileName());
    }
    udp.close();
}

void BootLoader::stopLoad()
{
    mutex.lock();
    stopFlag = true;
    mutex.unlock();
}
