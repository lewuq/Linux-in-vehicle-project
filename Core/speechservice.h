#pragma once
#include "speechrecognition.h"
#include <QNetworkAccessManager>
#include <QObject>
enum class VoiceCommand {
    Unknown,
    Home,
    Music,
    MusicPause,
    MusicPlay,
    MusicNext,
    MusicPrevious,
    Map,
    MapZoomIn,
    MapZoomOut,
    Weather,
    Camera,
    Video,
    VideoPause,
    VideoPlay,
    VideoNext,
    VideoPrevious,
    Settings
};
VoiceCommand parseVoiceCommand(QString text);
class SpeechService : public QObject {
    Q_OBJECT
public:
    explicit SpeechService(QObject *parent = nullptr);
    void start();
    void stop();
signals:
    void command(VoiceCommand command);
    void status(QString text);

private:
    void recognize();
    SpeechRecognition recorder;
    QNetworkAccessManager network;
    bool recognizing = false;
};
