#include "speechrecognition.h"
#include "Core/appconfig.h"
#include <QFile>
#include <QUrl>
#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <unistd.h>
#endif
SpeechRecognition::SpeechRecognition(QObject *parent) : QObject(parent) {
    connect(&recorder, &QMediaRecorder::stateChanged, this, [this](QMediaRecorder::State state) {
        if (state == QMediaRecorder::StoppedState && pending) {
            pending = false;
            emit RecordFinished();
        }
    });
    connect(&recorder, QOverload<QMediaRecorder::Error>::of(&QMediaRecorder::error), this,
            [this](QMediaRecorder::Error) {
                pending = false;
                emit status(QStringLiteral("录音失败，请检查麦克风"));
            });
#ifdef Q_OS_LINUX
    if (AppConfig::hardwareEnabled())
        keyFd = open(AppConfig::value("hardware/key_device", "/dev/key").toLocal8Bit().constData(),
                     O_RDONLY | O_NONBLOCK);
    if (keyFd >= 0) {
        keyTimer.setInterval(50);
        connect(&keyTimer, &QTimer::timeout, this, &SpeechRecognition::readKey);
        keyTimer.start();
    }
#endif
}
SpeechRecognition::~SpeechRecognition() {
    pending = false;
    recorder.stop();
#ifdef Q_OS_LINUX
    if (keyFd >= 0)
        close(keyFd);
#endif
}
void SpeechRecognition::readKey() {
#ifdef Q_OS_LINUX
    unsigned char value = 0;
    if (read(keyFd, &value, 1) != 1)
        return;
    const bool pressed = value == 240;
    if (pressed && !keyPressed)
        startRecord();
    else if (!pressed && keyPressed)
        stopRecord();
    keyPressed = pressed;
#endif
}
void SpeechRecognition::startRecord() {
    if (recorder.state() == QMediaRecorder::RecordingState || pending)
        return;
    if (recorder.audioInputs().isEmpty()) {
        emit status(QStringLiteral("未检测到麦克风"));
        return;
    }
    const auto codecs = recorder.supportedAudioCodecs(),
               containers = recorder.supportedContainers();
    QString codec, container;
    for (const auto &item : codecs)
        if (item.contains("pcm", Qt::CaseInsensitive)) {
            codec = item;
            break;
        }
    for (const auto &item : containers)
        if (item.contains("wav", Qt::CaseInsensitive)) {
            container = item;
            break;
        }
    if (codec.isEmpty() || container.isEmpty()) {
        emit status(QStringLiteral("录音设备不支持 PCM/WAV"));
        return;
    }
    QAudioEncoderSettings settings;
    settings.setCodec(codec);
    settings.setSampleRate(16000);
    settings.setChannelCount(1);
    settings.setBitRate(256000);
    recorder.setEncodingSettings(settings, QVideoEncoderSettings(), container);
    QFile::remove(AppConfig::recordingPath());
    recorder.setOutputLocation(QUrl::fromLocalFile(AppConfig::recordingPath()));
    recorder.record();
    emit status(QStringLiteral("正在聆听，松开结束"));
}
void SpeechRecognition::stopRecord() {
    if (recorder.state() != QMediaRecorder::RecordingState)
        return;
    pending = true;
    recorder.stop();
}
