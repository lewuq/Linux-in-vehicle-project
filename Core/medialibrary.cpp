#include "medialibrary.h"
#include "appconfig.h"
#include <QCollator>
#include <QDir>
#include <QSet>
#include <QtConcurrent>
#include <algorithm>
MediaLibrary::MediaLibrary(QObject *parent) : QObject(parent) {
    connect(&watcher, &QFutureWatcher<QVector<MediaTrack>>::finished, this, [this]() {
        scanning = false;
        const QString pending = pendingKind;
        pendingKind.clear();
        emit scanned(watcher.result());
        if (!pending.isEmpty())
            scan(pending);
    });
}
void MediaLibrary::scan(const QString &kind) {
    if (scanning) {
        pendingKind = kind;
        return;
    }
    scanning = true;
    const auto directories = AppConfig::mediaDirectories(kind);
    const QStringList filters = kind == "music" ? QStringList{"*.mp3", "*.wav", "*.flac", "*.ogg"}
                                                : QStringList{"*.mp4", "*.mkv", "*.avi", "*.mov"};
    watcher.setFuture(QtConcurrent::run([directories, filters]() {
        QVector<MediaTrack> tracks;
        QSet<QString> seen;
        for (const auto &directory : directories) {
            const auto files = QDir(directory).entryInfoList(filters, QDir::Files | QDir::Readable);
            for (const auto &file : files) {
                const QString path = file.canonicalFilePath();
                if (!path.isEmpty() && !seen.contains(path)) {
                    seen.insert(path);
                    tracks.append({file.completeBaseName(), path});
                }
            }
        }
        QCollator collator;
        collator.setNumericMode(true);
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(tracks.begin(), tracks.end(),
                  [&collator](const MediaTrack &a, const MediaTrack &b) {
                      return collator.compare(a.title, b.title) < 0;
                  });
        return tracks;
    }));
}
