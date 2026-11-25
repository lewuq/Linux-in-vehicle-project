#pragma once
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
namespace Network {
inline void prepare(QNetworkRequest &request) {
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "VehicleTerminal/Qt5");
}
inline void setTimeout(QNetworkReply *reply, int ms = 12000) {
    auto *timer = new QTimer(reply);
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, reply, [reply]() {
        reply->setProperty("timedOut", true);
        reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, timer, &QTimer::stop);
    timer->start(ms);
}
} // namespace Network
