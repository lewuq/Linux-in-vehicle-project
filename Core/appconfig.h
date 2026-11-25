#pragma once
#include <QString>
#include <QStringList>
namespace AppConfig {
QString value(const QString &key, const QString &fallback = QString());
QString dataDirectory();
QString recordingPath();
QStringList mediaDirectories(const QString &kind);
bool hardwareEnabled();
} // namespace AppConfig
