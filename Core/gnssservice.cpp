#include "gnssservice.h"
#include "appconfig.h"
GnssService::GnssService(QObject *parent) : QObject(parent) {
    timer.setInterval(1000);
    connect(&timer, &QTimer::timeout, this, &GnssService::refresh);
    setEnabled(AppConfig::hardwareEnabled());
}
GnssService::~GnssService() {
    gps_stop();
}
void GnssService::setEnabled(bool active) {
    timer.stop();
    gps_stop();
    current = GpsFix{};
    enabled = false;
    if (!active)
        message = QStringLiteral("定位未开启");
    else {
        const auto device = AppConfig::value("hardware/gnss_device", "/dev/ttymxc2").toLocal8Bit();
        const unsigned int baud = AppConfig::value("hardware/gnss_baud", "9600").toUInt();
        enabled = gps_start(device.constData(), baud) == 0;
        message = enabled ? QStringLiteral("正在搜索卫星") : QStringLiteral("定位设备不可用");
        if (enabled)
            timer.start();
    }
    emit updated();
}
void GnssService::refresh() {
    gps_snapshot(&current);
    message = current.valid ? QStringLiteral("已定位 · %1 颗卫星").arg(current.satellites)
                            : QStringLiteral("正在搜索卫星");
    emit updated();
}
