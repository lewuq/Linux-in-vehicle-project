/* Copyright © Deng Zhimao Co., Ltd. 1990-2030. All rights reserved.
 * Original sensor example adapted to length-bounded Qt file reads. */
#include "ap3216.h"
#include "Core/appconfig.h"
#include <QFile>
QString Ap3216c::readValue(const QString &name) {
    if (!AppConfig::hardwareEnabled())
        return QString();
    QFile file(AppConfig::value("hardware/proximity_path", "/sys/class/misc/ap3216c") + "/" + name);
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    const auto bytes = file.read(32);
    bool ok = false;
    const auto text = QString::fromLatin1(bytes).trimmed();
    text.toInt(&ok);
    return ok ? text : QString();
}
QString Ap3216c::readPsData() {
    return readValue("ps");
}
QString Ap3216c::readIrData() {
    return readValue("ir");
}
QString Ap3216c::readAlsData() {
    return readValue("als");
}
