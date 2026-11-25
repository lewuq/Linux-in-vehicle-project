#include "monitor.h"
#include "Ui/uikit.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
Monitor::Monitor(QWidget *parent) : QMainWindow(parent), capture(this), sensor(this) {
    setObjectName("page");
    auto *page = new QWidget;
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(8, 4, 8, 8);
    setCentralWidget(page);
    auto *top = new QHBoxLayout;
    top->addWidget(UiKit::label(QStringLiteral("摄像头影像"), "title"));
    top->addStretch();
    fps = UiKit::label("-- FPS", "muted");
    toggle = UiKit::button(QStringLiteral("启动影像"), "camera");
    top->addWidget(fps);
    top->addWidget(toggle);
    root->addLayout(top);
    auto *card = UiKit::card();
    auto *layout = new QVBoxLayout(card);
    image = UiKit::label(QStringLiteral("影像未开启"));
    image->setAlignment(Qt::AlignCenter);
    image->setMinimumSize(320, 220);
    layout->addWidget(image, 1);
    root->addWidget(card, 1);
    auto *bottom = new QHBoxLayout;
    status = UiKit::label(QStringLiteral("等待摄像头"), "muted");
    status->setWordWrap(true);
    proximity = UiKit::label(QStringLiteral("接近度 --"), "muted");
    bottom->addWidget(status, 1);
    bottom->addWidget(proximity);
    root->addLayout(bottom);
    connect(toggle, &QPushButton::clicked, this, [this]() {
        if (wanted)
            myStop();
        else
            myStart();
    });
    connect(&capture, &CaptureThread::imageReady, this, [this](QImage frame) {
        if (wanted) {
            image->setPixmap(QPixmap::fromImage(frame).scaled(image->size(), Qt::KeepAspectRatio,
                                                              Qt::FastTransformation));
            status->setText(QStringLiteral("实时影像"));
            ++frames;
            if (fpsTimer.elapsed() >= 1000) {
                fps->setText(
                    QString("%1 FPS").arg(frames * 1000.0 / fpsTimer.elapsed(), 0, 'f', 1));
                frames = 0;
                fpsTimer.restart();
            }
        }
        capture.acknowledgeFrame();
    });
    connect(&capture, &CaptureThread::captureError, this, [this](QString error) {
        wanted = false;
        timer.stop();
        image->clear();
        image->setText(error);
        status->setText(error);
        toggle->setText(QStringLiteral("重试连接"));
    });
    connect(&capture, &QThread::finished, this, [this]() {
        if (wanted)
            capture.startThread();
    });
    timer.setInterval(1000);
    connect(&timer, &QTimer::timeout, this, [this]() {
        const QString value = sensor.readPsData();
        proximity->setText(value.isEmpty() ? QStringLiteral("接近度传感器不可用")
                                           : QStringLiteral("接近度原始值 %1").arg(value));
    });
}
Monitor::~Monitor() {
    myStop();
    capture.wait();
}
void Monitor::myStart() {
    if (wanted)
        return;
    wanted = true;
    frames = 0;
    fpsTimer.start();
    status->setText(QStringLiteral("正在连接摄像头…"));
    toggle->setText(QStringLiteral("停止影像"));
    timer.start();
    capture.startThread();
}
void Monitor::myStop() {
    wanted = false;
    timer.stop();
    capture.stopThread();
    fps->setText("-- FPS");
    status->setText(QStringLiteral("影像已暂停"));
    toggle->setText(QStringLiteral("启动影像"));
}
