#pragma once
#include <QFutureWatcher>
#include <QObject>
#include <QVector>
struct MediaTrack {
    QString title;
    QString path;
};
class MediaLibrary : public QObject {
    Q_OBJECT
public:
    explicit MediaLibrary(QObject *parent = nullptr);
    void scan(const QString &kind);
    bool isScanning() const {
        return scanning;
    }
signals:
    void scanned(QVector<MediaTrack> tracks);

private:
    QFutureWatcher<QVector<MediaTrack>> watcher;
    QString pendingKind;
    bool scanning = false;
};
