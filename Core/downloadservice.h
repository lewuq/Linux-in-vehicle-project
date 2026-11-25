#pragma once
#include <QNetworkAccessManager>
#include <QObject>
class DownloadService : public QObject {
    Q_OBJECT
public:
    explicit DownloadService(QObject *parent = nullptr, QNetworkAccessManager *transport = nullptr)
        : QObject(parent), transport(transport ? transport : &network) {}
    void download(const QString &title, const QUrl &url);
signals:
    void status(QString message);
    void saved(QString path);

private:
    QNetworkAccessManager network;
    QNetworkAccessManager *transport;
};
