#include "appconfig.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
QString AppConfig::value(const QString &key, const QString &fallback) {
    const QByteArray env = ("VEHICLE_" + QString(key).replace('/', '_').toUpper()).toLatin1();
    if (qEnvironmentVariableIsSet(env.constData()))
        return qEnvironmentVariable(env.constData());
    QSettings config(qEnvironmentVariable("VEHICLE_CONFIG",
                                          QCoreApplication::applicationDirPath() + "/vehicle.ini"),
                     QSettings::IniFormat);
    config.setIniCodec("UTF-8");
    return config.value(key, fallback).toString();
}
QString AppConfig::dataDirectory() {
    const QString path =
        value("paths/data", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QDir().mkpath(path);
    return path;
}
QString AppConfig::recordingPath() {
    return dataDirectory() + "/record.wav";
}
QStringList AppConfig::mediaDirectories(const QString &kind) {
    QStringList dirs = value("paths/" + kind).split(';', Qt::SkipEmptyParts);
    dirs << dataDirectory() + "/" + kind
         << QCoreApplication::applicationDirPath() + "/" +
                (kind == "music" ? "Music/myMusic" : "Video");
#ifdef Q_OS_LINUX
    dirs << "/run/media/mmcblk0p1/" + kind << "/run/media/mmcblk0p1" << "/media/sdcard/" + kind;
#endif
    dirs.removeDuplicates();
    return dirs;
}
bool AppConfig::hardwareEnabled() {
#if defined(Q_OS_LINUX) && (defined(__arm__) || defined(__aarch64__))
    return value("hardware/enabled", "true") == "true";
#else
    return value("hardware/enabled", "false") == "true";
#endif
}
