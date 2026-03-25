#ifndef SETTINGWINDOW_H
#define SETTINGWINDOW_H

#include <QMainWindow>
#include <QMovie>
#include <QDir>
#include <QFile>
#include <QMediaObject>
#include <QTimer>
#include <QTime>
#include <QDate>
#include <QProcess>
#include <QDebug>
#include <QMessageBox>
#include <QLabel>
#include <QHash>
#include <QPixmap>
#include <QSize>

namespace Ui {
class SettingWindow;
}

class SettingWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SettingWindow(QWidget *parent = nullptr);
    ~SettingWindow();
    void ScanGif();
    void showTimeSettingPanel();
private slots:
    void on_pushButton_8_clicked();

    void on_pBtn_PauseGif_clicked(bool checked);

    void on_pBtn_SwitchGif_clicked();

    void on_pBtn_ModifyDate_clicked();

    void on_pBtn_ModifyTime_clicked();
    void on_timer_updateTime();
    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();
//    void on_comboBox_city_currentTextChanged(const QString &arg1);

private:
    bool applySystemDateTime(const QString &dateTimeText);
    void refreshMemoryInfo();
    QString runCommand(const QString &program, const QStringList &arguments, QString *errorOut = nullptr);
    QString readGnssNmeaSnapshot();
    bool parseCoordinatesFromNmea(const QString &nmeaText, double *lat, double *lon) const;
    double nmeaToDecimal(const QString &value, bool isLatitude, const QString &hemisphere) const;
    QString reverseGeocode(double lat, double lon);
    QString baiduStaticMapUrl(const QString &center, const QSize &size) const;
    void updateGnssMapPreview(const QString &center);
    void refreshGnssInfo();
    Ui::SettingWindow *ui;
    QString LocalGifPath="/home/root/mytest/MyGif";
    QMovie *movie = new QMovie;
    QVector<QString> gif_Files;
    int currentGifIndex=0;
    int GifSum=0;
    QTimer *timer;
    QProcess process;
    QLabel *wifiStatusLabel = nullptr;
    QLabel *memoryInfoLabel = nullptr;
    QLabel *gnssStatusLabel = nullptr;
    QLabel *gnssCoordLabel = nullptr;
    QLabel *gnssLocationLabel = nullptr;
    QLabel *gnssRawLabel = nullptr;
    QLabel *gnssMapLabel = nullptr;
    QWidget *gnssPageWidget = nullptr;
    QHash<QString, QString> wifiServiceMap;

};

#endif // SETTINGWINDOW_H
