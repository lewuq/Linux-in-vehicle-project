#pragma once
#include "Core/downloadservice.h"
#include "Core/medialibrary.h"
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QPushButton>
#include <QSlider>
class SearchMusic;
class MusicPlayer : public QMainWindow {
    Q_OBJECT
public:
    explicit MusicPlayer(QWidget *parent = nullptr);
    void ScanLocalSongs();
    QString currentTitle() const;
    bool hasTracks() const {
        return !tracks.isEmpty();
    }
    bool isPlaying() const {
        return player.state() == QMediaPlayer::PlayingState;
    }
    int volume() const {
        return player.volume();
    }
    void setVolume(int value) {
        player.setVolume(qBound(0, value, 100));
    }
    void pause() {
        player.pause();
    }
    void play();
public slots:
    void togglePlayback();
    void next();
    void previous();
    void playRandomSong();
signals:
    void nowPlayingChanged(QString title, bool playing);
    void volumeChanged(int volume);

private:
    void rebuildList();
    void updateCurrent();
    void toggleFavorite();
    QMediaPlayer player;
    QMediaPlaylist playlist;
    MediaLibrary library;
    DownloadService downloads;
    QVector<MediaTrack> tracks;
    QStringList favorites;
    QListWidget *list;
    QLabel *title, *subtitle, *time, *status;
    QSlider *seek, *volumeSlider;
    QPushButton *playButton, *favoriteButton, *filterButton, *modeButton, *scanButton;
    SearchMusic *search = nullptr;
    bool restoring = true;
    qint64 resumePosition = 0;
};
