#include "timeservice.h"
#include "network.h"
#include <QTimeZone>
void TimeService::finish(const QString &text) {
    working = false;
    emit status(text, false);
}
void TimeService::setTime(const QDateTime &time) {
    if (working || !time.isValid())
        return;
    working = true;
    emit status(QStringLiteral("正在设置时间…"), true);
    apply(time);
}
void TimeService::apply(const QDateTime &time) {
#ifdef Q_OS_LINUX
    runner.run("date", {"-u", "-s", time.toUTC().toString("yyyy-MM-dd HH:mm:ss")},
               [this](CommandResult result) {
                   if (!result.ok) {
                       finish(QStringLiteral("设置时间失败：权限不足或命令不可用"));
                       return;
                   }
                   runner.run("hwclock", {"-w", "-u"}, [this](CommandResult rtc) {
                       finish(rtc.ok ? QStringLiteral("系统时间和硬件时钟已更新")
                                     : QStringLiteral("系统时间已更新，硬件时钟写入失败"));
                   });
               });
#else
    Q_UNUSED(time);
    finish(QStringLiteral("桌面预览不修改系统时间"));
#endif
}
void TimeService::sync() {
    if (working)
        return;
    working = true;
    emit status(QStringLiteral("正在同步网络时间…"), true);
    QNetworkRequest request(QUrl("https://www.baidu.com"));
    Network::prepare(request);
    auto *reply = network.head(request);
    Network::setTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QDateTime time =
            QDateTime::fromString(QString::fromLatin1(reply->rawHeader("Date")), Qt::RFC2822Date);
        const bool ok = reply->error() == QNetworkReply::NoError && time.isValid();
        reply->deleteLater();
        if (!ok) {
            finish(QStringLiteral("网络校时失败，请稍后重试"));
            return;
        }
        apply(time);
    });
}
