#include "systemstatus.h"
#include "appconfig.h"
#include <QFile>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QStorageInfo>
QString SystemStatus::network() {
    for (const auto &interface : QNetworkInterface::allInterfaces()) {
        const auto flags = interface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning) ||
            (flags & QNetworkInterface::IsLoopBack))
            continue;
        for (const auto &address : interface.addressEntries())
            if (!address.ip().isLoopback() && !address.ip().isNull())
                return QStringLiteral("网络已连接");
    }
    return QStringLiteral("网络未连接");
}
QString SystemStatus::memory() {
    QFile file("/proc/meminfo");
    if (!file.open(QIODevice::ReadOnly))
        return QStringLiteral("当前平台不提供内存数据");
    const QString text = QString::fromLatin1(file.readAll());
    auto number = [text](const QString &key) {
        return QRegularExpression(key + ":\\s+(\\d+)").match(text).captured(1).toLongLong();
    };
    const qint64 total = number("MemTotal"), available = number("MemAvailable");
    if (total <= 0)
        return QStringLiteral("内存数据不可用");
    return QStringLiteral("%1 / %2 MB 已使用").arg((total - available) / 1024).arg(total / 1024);
}
QString SystemStatus::uptime() {
    QFile file("/proc/uptime");
    if (!file.open(QIODevice::ReadOnly))
        return QStringLiteral("桌面预览");
    const auto seconds = file.readAll().split(' ').value(0).toDouble();
    return QStringLiteral("%1 小时 %2 分钟").arg(int(seconds) / 3600).arg((int(seconds) / 60) % 60);
}
QString SystemStatus::storage() {
    QStorageInfo storage(AppConfig::dataDirectory());
    if (!storage.isValid() || !storage.isReady())
        return QStringLiteral("存储不可用");
    return QStringLiteral("%1 GB 可用 / %2 GB")
        .arg(storage.bytesAvailable() / 1073741824.0, 0, 'f', 1)
        .arg(storage.bytesTotal() / 1073741824.0, 0, 'f', 1);
}
