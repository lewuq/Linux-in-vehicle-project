/* V4L2 capture adapted from Deng Zhimao / ALIENTEK, 2021.
 * Original copyright notices are retained in THIRD_PARTY_NOTICES.md. */
#pragma once
#include <QImage>
#include <QThread>
#include <atomic>
class CaptureThread : public QThread {
    Q_OBJECT
public:
    explicit CaptureThread(QObject *parent = nullptr) : QThread(parent) {}
    ~CaptureThread() override {
        requestInterruption();
        wait();
    }
    void startThread() {
        if (!isRunning())
            start();
    }
    void stopThread() {
        requestInterruption();
    }
    void acknowledgeFrame() {
        framePending = false;
    }
signals:
    void imageReady(QImage image);
    void captureError(QString message);

protected:
    void run() override;

private:
    std::atomic<bool> framePending{false};
};
