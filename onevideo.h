#pragma once
#include "Core/medialibrary.h"
#include <QMediaPlayer>
#include <QWidget>
class QVideoWidget;
class QListWidget;
class QLabel;
class QSlider;
class QPushButton;
class OneVideo : public QWidget {
    Q_OBJECT
public:
    explicit OneVideo(QWidget *parent = nullptr);
    void ScanLocalVideos();
    void play();
    void pause() {
        player.pause();
    }
    void setVolume(int value) {
        player.setVolume(value);
    }
    void onNextVideo();
    void onPrevVideo();
signals:
    void volumeChanged(int value);

private:
    void select(int index, bool start);
    void updateControls();
    QMediaPlayer player;
    MediaLibrary library;
    QVector<MediaTrack> tracks;
    int current = -1;
    QVideoWidget *video;
    QListWidget *list;
    QLabel *title, *status, *time;
    QSlider *seek, *volume;
    QPushButton *playButton, *scanButton;
};
