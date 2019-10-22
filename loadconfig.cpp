#include "loadconfig.h"
#include <QRegExp>

bool LoadConfig::setIP(const QString &value)
{
    QString ip_str = value;
    ip_str.remove("IP:");
    ip_str.remove("ip:");
    ip_str = ip_str.trimmed();
    QRegExp exp("^(\\d{1,3})\\.(\\d{1,3}).(\\d{1,3}).(\\d{1,3})");
    if(exp.indexIn(ip_str)!=-1) {
        ip=ip_str;
        return true;
    }
    ip="";
    return false;
}

bool LoadConfig::setGroup(int value)
{
    const int min=1;
    const int max=100;
    if(value>=min && value<=max) {
        group=value;
        return true;
    }
    return false;
}

bool LoadConfig::setPoint(int value)
{
    const int min=1;
    const int max=100;
    if(value>=min && value<=max) {
        point =value;
        return true;
    }
    return false;
}

LoadConfig::LoadConfig(const QString &ip, int group, int point)
{
    setIP(ip);
    setGroup(group);
    setPoint(point);
}

