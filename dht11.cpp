#include "dht11.h"
#include "Core/appconfig.h"
#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#endif
void Dht11::run() {
#ifdef Q_OS_LINUX
    if (!AppConfig::hardwareEnabled()) {
        emit unavailable(QStringLiteral("环境传感器未连接"));
        return;
    }
    const auto path = AppConfig::value("hardware/dht_device", "/dev/dht11").toLocal8Bit();
    const int fd = open(path.constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        emit unavailable(QStringLiteral("环境传感器不可用"));
        return;
    }
    while (!isInterruptionRequested()) {
        unsigned char data[5] = {0};
        struct pollfd descriptor = {fd, POLLIN, 0};
        int ready = poll(&descriptor, 1, 200);
        if (ready > 0 && (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))) {
            emit unavailable(QStringLiteral("环境传感器已断开"));
            break;
        }
        if (ready > 0 && read(fd, data, sizeof(data)) == sizeof(data)) {
            if (static_cast<unsigned char>(data[0] + data[1] + data[2] + data[3]) == data[4])
                emit updateDht11Data(QString("%1.%2").arg(data[0]).arg(data[1]),
                                     QString("%1.%2").arg(data[2]).arg(data[3]));
            else
                emit unavailable(QStringLiteral("环境数据校验失败"));
        }
        for (int i = 0; i < 10 && !isInterruptionRequested(); ++i)
            msleep(100);
    }
    close(fd);
#else
    emit unavailable(QStringLiteral("环境传感器未连接"));
#endif
}
