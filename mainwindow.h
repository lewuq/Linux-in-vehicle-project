#pragma once
#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>
#include <QVector>
class QStackedWidget;
class QPushButton;
class QLabel;
class QSlider;
class MusicPlayer;
class OneVideo;
class Monitor;
class BaiduMap;
class Weather;
class SettingWindow;
class GnssService;
class WeatherService;
class SpeechService;
class Dht11;
enum class VoiceCommand;
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void openPage(int index);
    void capturePages(const QString &directory);

private:
    QWidget *createHome();
    QWidget *ensurePage(int index);
    MusicPlayer *ensureMusic();
    void updateClock();
    void updateStatus();
    void handleVoice(VoiceCommand command);
    void updateWeather();
    QStackedWidget *stack;
    QVector<QWidget *> pages;
    QVector<QPushButton *> navigation;
    QLabel *pageTitle, *clockLabel, *dateLabel, *networkLabel, *positionLabel, *weatherLabel,
        *weatherDetail, *weatherImage, *temperatureLabel, *humidityLabel, *sensorLabel, *songLabel,
        *footerSong, *sessionLabel, *voiceLabel;
    QPushButton *homePlay, *footerPlay;
    QSlider *volumeSlider;
    MusicPlayer *music = nullptr;
    OneVideo *video = nullptr;
    Monitor *camera = nullptr;
    BaiduMap *map = nullptr;
    SettingWindow *settings = nullptr;
    GnssService *gnss;
    WeatherService *weather;
    SpeechService *speech;
    Dht11 *sensor;
    QElapsedTimer session;
    QTimer ticker;
    int currentPage = 0;
};
