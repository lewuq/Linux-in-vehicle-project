#pragma once
#include "ap3216.h"
#include "capture_thread.h"
#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>
class QLabel;
class QPushButton;
class Monitor : public QMainWindow {
    Q_OBJECT
public:
    explicit Monitor(QWidget *parent = nullptr);
    ~Monitor() override;
    void myStart();
    void myStop();

private:
    CaptureThread capture;
    Ap3216c sensor;
    QTimer timer;
    QElapsedTimer fpsTimer;
    int frames = 0;
    bool wanted = false;
    QLabel *image, *status, *proximity, *fps;
    QPushButton *toggle;
};
