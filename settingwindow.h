#pragma once
#include <QMainWindow>
#include <QTimer>
class GnssService;
class WifiService;
class TimeService;
class QListWidget;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class SettingWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit SettingWindow(GnssService *gnss, QWidget *parent = nullptr);
    void setVolume(int value);
signals:
    void volumeRequested(int value);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void renderWifi();
    void renderSystem();
    GnssService *gnss;
    WifiService *wifi;
    TimeService *clock;
    QTimer refresh;
    QListWidget *networks;
    QLineEdit *password;
    QLabel *wifiStatus, *gnssStatus, *memory, *storage, *uptime, *timeStatus;
    QPushButton *scanButton, *connectButton, *onButton, *offButton, *syncButton, *applyButton;
    QSlider *volumeSlider;
};
