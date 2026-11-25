#include "downloadservice.h"
#include "appconfig.h"
#include "network.h"
#include <QDir>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUuid>
#include <memory>
namespace {
struct Task {
    QSaveFile file;
    QByteArray prefix;
    qint64 bytes = 0;
    bool failed = false;
    QString path;
};
bool audioSignature(const QByteArray &data) {
    return data.startsWith("ID3") || data.startsWith("RIFF") || data.startsWith("fLaC") ||
           data.startsWith("OggS") ||
           (data.size() >= 2 && static_cast<unsigned char>(data[0]) == 0xff &&
            (static_cast<unsigned char>(data[1]) & 0xe0) == 0xe0);
}
} // namespace
void DownloadService::download(const QString &title, const QUrl &url) {
    if (url.scheme() != "https" || url.host().isEmpty()) {
        emit status(QStringLiteral("下载地址必须使用 HTTPS"));
        return;
    }
    const QString directory = AppConfig::dataDirectory() + "/music";
    if (!QDir().mkpath(directory)) {
        emit status(QStringLiteral("音乐目录不可写"));
        return;
    }
    QString safe = title;
    safe.replace(QRegularExpression("[\\\\/:*?\"<>|\\x00-\\x1f]"), "_");
    safe = safe.left(100).trimmed();
    if (safe.isEmpty())
        safe = QStringLiteral("未命名歌曲");
    const auto task = std::make_shared<Task>();
    task->path =
        directory + "/" + safe + "-" + QUuid::createUuid().toString(QUuid::Id128).left(8) + ".mp3";
    task->file.setFileName(task->path);
    if (!task->file.open(QIODevice::WriteOnly)) {
        emit status(QStringLiteral("无法创建下载文件"));
        return;
    }
    QNetworkRequest request(url);
    Network::prepare(request);
    auto *reply = transport->get(request);
    reply->setReadBufferSize(64 * 1024);
    Network::setTimeout(reply, 60000);
    auto drain = [reply, task]() {
        const auto chunk = reply->readAll();
        task->bytes += chunk.size();
        if (task->prefix.size() < 512)
            task->prefix.append(chunk.left(512 - task->prefix.size()));
        if (task->bytes > 64 * 1024 * 1024 || task->file.write(chunk) != chunk.size()) {
            task->failed = true;
            reply->abort();
        }
    };
    connect(reply, &QNetworkReply::readyRead, this, drain);
    connect(reply, &QNetworkReply::finished, this, [this, reply, task, drain]() {
        drain();
        const bool valid = !task->failed && reply->error() == QNetworkReply::NoError &&
                           audioSignature(task->prefix);
        const bool committed = valid && task->file.commit();
        if (!committed) {
            task->file.cancelWriting();
            emit status(QStringLiteral("下载失败：网络、权限或音频内容异常"));
        } else {
            emit saved(task->path);
            emit status(QStringLiteral("歌曲已添加到本地音乐"));
        }
        reply->deleteLater();
    });
    emit status(QStringLiteral("正在下载 %1").arg(title));
}
