#ifndef LOADCONFIG_H
#define LOADCONFIG_H

#include <QString>

class LoadConfig
{
    QString ip;
    int group;
    int point;
    QString fName;
public:
    bool setIP(const QString &value);
    bool setGroup(int value);
    bool setPoint(int value);
    void setFileName(const QString &value) {fName = value;}
    LoadConfig(const QString &ip,int group,int point);
    QString getIP() const {return ip;}
    int getGroup() const {return group;}
    int getPoint() const {return point;}
    QString getFileName() const {return fName;}
};

#endif // LOADCONFIG_H
