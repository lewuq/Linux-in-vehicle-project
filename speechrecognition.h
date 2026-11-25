#pragma once
#include <QAudioRecorder>
#include <QObject>
#include <QTimer>
class SpeechRecognition : public QObject {
    Q_OBJECT
public:
    explicit SpeechRecognition(QObject *parent = nullptr);
    ~SpeechRecognition() override;
    void startRecord();
    void stopRecord();
signals:
    void RecordFinished();
    void status(QString text);

private:
    void readKey();
    QAudioRecorder recorder;
    QTimer keyTimer;
    int keyFd = -1;
    bool keyPressed = false, pending = false;
};
