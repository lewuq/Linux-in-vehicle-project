#include "mainwindow.h"
#include "Core/gnssservice.h"
#include "Core/i18n.h"
#include "Core/speechservice.h"
#include "Core/systemstatus.h"
#include "Core/weatherservice.h"
#include "Map/baidumap.h"
#include "Monitor/monitor.h"
#include "Music/musicplayer.h"
#include "Ui/uikit.h"
#include "Weather/weather.h"
#include "dht11.h"
#include "onevideo.h"
#include "settingwindow.h"
#include <QApplication>
#include <QDir>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLocale>
#include <QSettings>
#include <QSharedPointer>
#include <QSignalBlocker>
#include <QSlider>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>
static const QStringList names = {
    QStringLiteral("首页"), QStringLiteral("地图"), QStringLiteral("音乐"), QStringLiteral("视频"),
    QStringLiteral("天气"), QStringLiteral("影像"), QStringLiteral("设置")};
static const QStringList symbols = {"home",    "map",    "music",   "video",
                                    "weather", "camera", "settings"};
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), pages(7, nullptr), gnss(new GnssService(this)),
      weather(new WeatherService(this)), speech(new SpeechService(this)), sensor(new Dht11(this)) {
    setObjectName("vehicleTerminal");
    resize(1024, 600);
    session.start();
    auto *shell = new QWidget;
    shell->setObjectName("shell");
    setCentralWidget(shell);
    auto *root = new QHBoxLayout(shell);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    auto *rail = new QFrame;
    rail->setObjectName("rail");
    rail->setFixedWidth(84);
    auto *nav = new QVBoxLayout(rail);
    nav->setContentsMargins(10, 18, 10, 14);
    nav->setSpacing(5);
    auto *brand = UiKit::label(QString());
    brand->setAlignment(Qt::AlignCenter);
    UiKit::setIcon(brand, "car", "home", 30);
    nav->addWidget(brand);
    nav->addSpacing(14);
    for (int i = 0; i < names.size(); ++i) {
        auto *button = new QPushButton;
        button->setObjectName("nav_" + symbols[i]);
        button->setProperty("nav", true);
        UiKit::setAccent(button, symbols[i]);
        button->setCheckable(true);
        button->setFixedSize(64, 62);
        I18n::setProperty(button, "accessibleName", names[i]);
        auto *column = new QVBoxLayout(button);
        column->setContentsMargins(2, 4, 2, 4);
        column->setSpacing(2);
        auto *icon = UiKit::label(QString());
        UiKit::setIcon(icon, symbols[i], symbols[i], 30, true);
        icon->setFixedSize(30, 30);
        icon->setAlignment(Qt::AlignCenter);
        auto *text = UiKit::label(names[i], "muted");
        text->setAlignment(Qt::AlignCenter);
        icon->setAttribute(Qt::WA_TransparentForMouseEvents);
        text->setAttribute(Qt::WA_TransparentForMouseEvents);
        column->addWidget(icon, 0, Qt::AlignHCenter);
        column->addWidget(text);
        nav->addWidget(button);
        navigation.append(button);
        connect(button, &QPushButton::clicked, this, [this, i]() { openPage(i); });
    }
    nav->addStretch();
    root->addWidget(rail);
    auto *content = new QVBoxLayout;
    content->setContentsMargins(22, 0, 22, 0);
    content->setSpacing(0);
    root->addLayout(content, 1);
    auto *header = new QFrame;
    header->setObjectName("header");
    header->setFixedHeight(70);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 8, 4, 8);
    pageTitle = UiKit::label(QStringLiteral("车载空间"), "heading");
    headerLayout->addWidget(pageTitle);
    headerLayout->addStretch();
    networkLabel = UiKit::label(QString(), "muted");
    positionLabel = UiKit::label(QString(), "badge");
    headerLayout->addWidget(networkLabel);
    headerLayout->addSpacing(16);
    headerLayout->addWidget(positionLabel);
    content->addWidget(header);
    stack = new QStackedWidget;
    pages[0] = createHome();
    stack->addWidget(pages[0]);
    content->addWidget(stack, 1);
    auto *footer = new QFrame;
    footer->setObjectName("footer");
    UiKit::setAccent(footer, "music");
    footer->setFixedHeight(72);
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(4, 10, 4, 10);
    auto *note = UiKit::label(QString());
    UiKit::setIcon(note, "music", "music", 28, true);
    footerLayout->addWidget(note);
    footerSong = UiKit::label(QStringLiteral("音乐，让旅途慢下来"), "muted");
    footerSong->setMaximumWidth(270);
    footerLayout->addWidget(footerSong, 1);
    footerPlay = UiKit::button(QString(), "play");
    footerPlay->setFixedWidth(50);
    UiKit::setPrimary(footerPlay, "music");
    footerLayout->addWidget(footerPlay);
    auto *volume = UiKit::button(QString(), "volume");
    volume->setCheckable(true);
    I18n::setProperty(volume, "toolTip", QStringLiteral("静音 / 恢复音量"));
    footerLayout->addWidget(volume);
    volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(QSettings().value("audio/volume", 35).toInt());
    volumeSlider->setFixedWidth(110);
    footerLayout->addWidget(volumeSlider);
    footerLayout->addSpacing(20);
    voiceLabel = UiKit::label(QStringLiteral("按住说话"), "muted");
    voiceLabel->setMaximumWidth(185);
    footerLayout->addWidget(voiceLabel);
    auto *mic = UiKit::button(QString(), "mic");
    I18n::setProperty(mic, "toolTip", QStringLiteral("按住录音，松开识别"));
    mic->setObjectName("voiceButton");
    UiKit::setAccent(mic, "video");
    footerLayout->addWidget(mic);
    content->addWidget(footer);
    connect(mic, &QPushButton::pressed, speech, &SpeechService::start);
    connect(mic, &QPushButton::released, speech, &SpeechService::stop);
    connect(speech, &SpeechService::status, this,
            [this](const QString &text) { I18n::setText(voiceLabel, text); });
    connect(speech, &SpeechService::command, this, &MainWindow::handleVoice);
    connect(footerPlay, &QPushButton::clicked, this, [this]() { ensureMusic()->togglePlayback(); });
    connect(volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        QSettings().setValue("audio/volume", value);
        if (music)
            music->setVolume(value);
        if (video)
            video->setVolume(value);
        if (settings)
            settings->setVolume(value);
    });
    connect(volume, &QPushButton::toggled, this, [this, volume](bool muted) {
        if (muted) {
            volume->setProperty("previous", qMax(1, volumeSlider->value()));
            volumeSlider->setValue(0);
        } else
            volumeSlider->setValue(volume->property("previous").toInt());
    });
    connect(sensor, &Dht11::updateDht11Data, this, [this](QString humidity, QString temperature) {
        I18n::setText(humidityLabel, humidity + " %");
        I18n::setText(temperatureLabel, temperature + " °C");
        I18n::setText(sensorLabel, QStringLiteral("环境传感器 · 实时"));
    });
    connect(sensor, &Dht11::unavailable, this, [this](QString reason) {
        I18n::setText(temperatureLabel, "-- °C");
        I18n::setText(humidityLabel, "-- %");
        I18n::setText(sensorLabel, reason);
    });
    sensor->start();
    connect(gnss, &GnssService::updated, this, &MainWindow::updateStatus);
    connect(weather, &WeatherService::updated, this, &MainWindow::updateWeather);
    connect(&I18n::instance(), &I18n::languageChanged, this, &MainWindow::updateClock);
    ticker.setInterval(1000);
    connect(&ticker, &QTimer::timeout, this, &MainWindow::updateClock);
    ticker.start();
    auto *statusTimer = new QTimer(this);
    statusTimer->setInterval(5000);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateStatus);
    statusTimer->start();
    openPage(0);
    updateClock();
    updateStatus();
    QTimer::singleShot(0, weather, &WeatherService::refresh);
    UiKit::refreshIcons(this);
}
MainWindow::~MainWindow() {
    sensor->requestInterruption();
    sensor->wait();
    if (camera)
        camera->myStop();
}
QWidget *MainWindow::createHome() {
    auto *home = new QWidget;
    home->setObjectName("homePage");
    auto *layout = new QVBoxLayout(home);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(14);
    auto *welcome = new QHBoxLayout;
    auto *intro = new QVBoxLayout;
    intro->setSpacing(4);
    intro->addWidget(UiKit::label(QStringLiteral("自在出发"), "title"));
    intro->addWidget(UiKit::label(QStringLiteral("熟悉的音乐，沿途的风景。"), "muted"));
    welcome->addLayout(intro);
    welcome->addStretch();
    sessionLabel = UiKit::label(QStringLiteral("本次使用 00:00"), "muted");
    welcome->addWidget(sessionLabel);
    layout->addLayout(welcome);
    auto *top = new QHBoxLayout;
    top->setSpacing(14);
    auto *hero = UiKit::card();
    hero->setObjectName("heroCard");
    UiKit::setAccent(hero, "home", true);
    auto *heroRow = new QHBoxLayout(hero);
    heroRow->setContentsMargins(24, 18, 16, 18);
    auto *clockColumn = new QVBoxLayout;
    clockColumn->setSpacing(0);
    clockColumn->addWidget(UiKit::label(QStringLiteral("日常旅途，舒适随行"), "eyebrow"));
    clockLabel = UiKit::label(QString(), "hero");
    dateLabel = UiKit::label(QString(), "body");
    clockColumn->addWidget(clockLabel);
    clockColumn->addWidget(dateLabel);
    heroRow->addLayout(clockColumn, 3);
    heroRow->addWidget(new RoadArtwork, 2);
    top->addWidget(hero, 5);
    auto *weatherCard = UiKit::card();
    UiKit::setAccent(weatherCard, "weather", true);
    auto *weatherLayout = new QVBoxLayout(weatherCard);
    weatherLayout->setContentsMargins(20, 18, 20, 18);
    auto *weatherTop = new QHBoxLayout;
    weatherTop->addWidget(UiKit::label(QStringLiteral("沿途天气"), "muted"));
    weatherTop->addStretch();
    weatherImage = UiKit::label(QString());
    UiKit::setIcon(weatherImage, "weather", "weather", 32, true);
    weatherTop->addWidget(weatherImage);
    weatherLayout->addLayout(weatherTop);
    weatherLabel = UiKit::label("--°", "metric");
    weatherDetail = UiKit::label(QStringLiteral("天气暂不可用"), "muted");
    weatherDetail->setWordWrap(true);
    weatherLayout->addWidget(weatherLabel);
    weatherLayout->addWidget(weatherDetail);
    weatherLayout->addStretch();
    top->addWidget(weatherCard, 2);
    layout->addLayout(top, 4);
    auto *quick = new QHBoxLayout;
    quick->setSpacing(12);
    const QVector<int> shortcuts = {1, 2, 3, 5};
    const QStringList captions = {QStringLiteral("地图与位置"), QStringLiteral("我的音乐"),
                                  QStringLiteral("本地视频"), QStringLiteral("摄像头影像")};
    for (int i = 0; i < shortcuts.size(); ++i) {
        const int index = shortcuts[i];
        auto *button = UiKit::button(captions[i], symbols[index]);
        button->setProperty("quick", true);
        UiKit::setAccent(button, symbols[index]);
        UiKit::setButtonIcon(button, symbols[index], symbols[index], 36, true);
        button->setMinimumHeight(66);
        connect(button, &QPushButton::clicked, this, [this, index]() { openPage(index); });
        quick->addWidget(button, 1);
    }
    layout->addLayout(quick);
    auto *bottom = new QHBoxLayout;
    bottom->setSpacing(14);
    auto *playing = UiKit::card();
    UiKit::setAccent(playing, "music", true);
    auto *playingLayout = new QVBoxLayout(playing);
    playingLayout->setContentsMargins(20, 16, 20, 16);
    auto *playingHeader = new QHBoxLayout;
    playingHeader->addWidget(UiKit::iconLabel("music", "music", 22));
    playingHeader->addWidget(UiKit::label(QStringLiteral("正在播放"), "muted"));
    playingHeader->addStretch();
    playingLayout->addLayout(playingHeader);
    songLabel = UiKit::label(QStringLiteral("留一点时间给喜欢的旋律"), "heading");
    songLabel->setWordWrap(true);
    playingLayout->addWidget(songLabel);
    auto *playRow = new QHBoxLayout;
    auto *previous = UiKit::button(QString(), "previous");
    homePlay = UiKit::button(QStringLiteral("播放音乐"), "play");
    UiKit::setPrimary(homePlay, "music");
    auto *next = UiKit::button(QString(), "next");
    playRow->addWidget(previous);
    playRow->addWidget(homePlay, 1);
    playRow->addWidget(next);
    playingLayout->addLayout(playRow);
    connect(previous, &QPushButton::clicked, this, [this]() { ensureMusic()->previous(); });
    connect(homePlay, &QPushButton::clicked, this, [this]() { ensureMusic()->togglePlayback(); });
    connect(next, &QPushButton::clicked, this, [this]() { ensureMusic()->next(); });
    bottom->addWidget(playing, 5);
    auto *environment = UiKit::card();
    UiKit::setAccent(environment, "climate", true);
    auto *envLayout = new QVBoxLayout(environment);
    envLayout->setContentsMargins(20, 16, 20, 16);
    auto *environmentHeader = new QHBoxLayout;
    environmentHeader->addWidget(UiKit::iconLabel("humidity", "climate", 22));
    environmentHeader->addWidget(UiKit::label(QStringLiteral("环境感知"), "muted"));
    environmentHeader->addStretch();
    envLayout->addLayout(environmentHeader);
    auto *metrics = new QHBoxLayout;
    temperatureLabel = UiKit::label("-- °C", "metric");
    humidityLabel = UiKit::label("-- %", "metric");
    metrics->addWidget(UiKit::iconLabel("temperature", "temperature", 24));
    UiKit::setAccent(temperatureLabel, "temperature");
    metrics->addWidget(temperatureLabel);
    metrics->addStretch();
    metrics->addWidget(UiKit::iconLabel("humidity", "humidity", 24));
    UiKit::setAccent(humidityLabel, "humidity");
    metrics->addWidget(humidityLabel);
    envLayout->addLayout(metrics);
    sensorLabel = UiKit::label(QStringLiteral("等待传感器"), "muted");
    sensorLabel->setWordWrap(true);
    envLayout->addWidget(sensorLabel);
    bottom->addWidget(environment, 4);
    layout->addLayout(bottom, 3);
    return home;
}
MusicPlayer *MainWindow::ensureMusic() {
    ensurePage(2);
    return music;
}
QWidget *MainWindow::ensurePage(int index) {
    if (index < 0 || index >= pages.size())
        return nullptr;
    if (pages[index])
        return pages[index];
    QWidget *page = nullptr;
    switch (index) {
    case 1:
        map = new BaiduMap(gnss);
        page = map;
        break;
    case 2:
        music = new MusicPlayer;
        page = music;
        music->setVolume(volumeSlider->value());
        connect(music, &MusicPlayer::nowPlayingChanged, this, [this](QString title, bool playing) {
            if (music->hasTracks()) {
                I18n::setLiteralText(songLabel, title);
                I18n::setLiteralText(
                    footerSong, footerSong->fontMetrics().elidedText(title, Qt::ElideRight, 260));
            } else {
                I18n::setText(songLabel, title);
                I18n::setText(footerSong, title);
            }
            footerSong->setToolTip(title);
            I18n::setText(homePlay,
                          playing ? QStringLiteral("暂停播放") : QStringLiteral("播放音乐"));
            UiKit::setButtonIcon(homePlay, playing ? "pause" : "play", "music");
            UiKit::setButtonIcon(footerPlay, playing ? "pause" : "play", "music");
        });
        connect(music, &MusicPlayer::volumeChanged, volumeSlider, &QSlider::setValue);
        break;
    case 3:
        video = new OneVideo;
        video->setVolume(volumeSlider->value());
        connect(video, &OneVideo::volumeChanged, volumeSlider, &QSlider::setValue);
        page = video;
        break;
    case 4:
        page = new Weather(weather);
        break;
    case 5:
        camera = new Monitor;
        page = camera;
        break;
    case 6:
        settings = new SettingWindow(gnss);
        page = settings;
        settings->setVolume(volumeSlider->value());
        connect(settings, &SettingWindow::volumeRequested, volumeSlider, &QSlider::setValue);
        break;
    default:
        return pages[0];
    }
    page->setWindowFlags(Qt::Widget);
    page->setMinimumSize(0, 0);
    pages[index] = page;
    stack->addWidget(page);
    return page;
}
void MainWindow::openPage(int index) {
    if (index < 0 || index >= pages.size())
        return;
    if (currentPage == 5 && camera && index != 5)
        camera->myStop();
    if (currentPage == 3 && video && index != 3)
        video->pause();
    QWidget *page = ensurePage(index);
    stack->setCurrentWidget(page);
    currentPage = index;
    for (int i = 0; i < navigation.size(); ++i)
        navigation[i]->setChecked(i == index);
    I18n::setText(pageTitle, index == 0 ? QStringLiteral("车载空间") : names[index]);
    if (index == 5) {
        if (music)
            music->pause();
        camera->myStart();
    }
    if (index == 3) {
        if (music)
            music->pause();
    }
    if (index == 2 && video)
        video->pause();
}
void MainWindow::updateClock() {
    const auto now = QDateTime::currentDateTime();
    I18n::setText(clockLabel, now.toString("HH:mm"));
    const QLocale locale = I18n::instance().locale();
    I18n::setLiteralText(dateLabel,
                         locale.toString(now.date(), I18n::instance().language() == "zh_CN"
                                                         ? QStringLiteral("M月d日  dddd")
                                                         : "ddd, MMM d"));
    const auto minutes = session.elapsed() / 60000;
    I18n::setText(sessionLabel, QStringLiteral("本次使用 %1:%2")
                                    .arg(minutes / 60, 2, 10, QChar('0'))
                                    .arg(minutes % 60, 2, 10, QChar('0')));
}
void MainWindow::updateStatus() {
    I18n::setText(networkLabel, SystemStatus::network());
    UiKit::setStatus(networkLabel,
                     SystemStatus::network() == QStringLiteral("网络已连接") ? "ready" : "idle");
    UiKit::setStatus(positionLabel, gnss->fix().valid ? "ready" : "idle");
    I18n::setText(positionLabel,
                  gnss->fix().valid ? QStringLiteral("定位已就绪") : QStringLiteral("定位未就绪"));
}
void MainWindow::updateWeather() {
    const auto &report = weather->report();
    if (report.days.isEmpty()) {
        I18n::setText(weatherLabel, "--°");
        I18n::setText(weatherDetail, QStringLiteral("天气暂不可用"));
        return;
    }
    const auto &day = report.days.first();
    I18n::setText(weatherLabel, day.high + "°");
    I18n::setText(weatherDetail,
                  report.city + " · " + day.description + "\n" + day.low + "° / " + day.high + "°");
}
void MainWindow::handleVoice(VoiceCommand command) {
    switch (command) {
    case VoiceCommand::Home:
        openPage(0);
        break;
    case VoiceCommand::Music:
        openPage(2);
        music->play();
        break;
    case VoiceCommand::MusicPause:
        if (music)
            music->pause();
        break;
    case VoiceCommand::MusicPlay:
        openPage(2);
        music->play();
        break;
    case VoiceCommand::MusicNext:
        ensureMusic()->next();
        break;
    case VoiceCommand::MusicPrevious:
        ensureMusic()->previous();
        break;
    case VoiceCommand::Map:
        openPage(1);
        break;
    case VoiceCommand::MapZoomIn:
        openPage(1);
        map->zoomIn();
        break;
    case VoiceCommand::MapZoomOut:
        openPage(1);
        map->zoomOut();
        break;
    case VoiceCommand::Weather:
        openPage(4);
        break;
    case VoiceCommand::Camera:
        openPage(5);
        break;
    case VoiceCommand::Video:
        openPage(3);
        video->play();
        break;
    case VoiceCommand::VideoPause:
        if (video)
            video->pause();
        break;
    case VoiceCommand::VideoPlay:
        openPage(3);
        video->play();
        break;
    case VoiceCommand::VideoNext:
        openPage(3);
        video->onNextVideo();
        break;
    case VoiceCommand::VideoPrevious:
        openPage(3);
        video->onPrevVideo();
        break;
    case VoiceCommand::Settings:
        openPage(6);
        break;
    default:
        break;
    }
}
void MainWindow::capturePages(const QString &directory) {
    QDir().mkpath(directory);
    auto *step = new QTimer(this);
    auto counter = QSharedPointer<int>::create(0);
    const QString originalLanguage = I18n::instance().language();
    connect(step, &QTimer::timeout, this, [this, step, counter, directory, originalLanguage]() {
        const QStringList captures = {"home",   "map",        "music",   "video",      "weather",
                                      "camera", "settings",   "display", "device",     "time",
                                      "night",  "large-text", "home-en", "settings-en"};
        if (*counter > 0)
            grab().save(directory + "/" + captures[*counter - 1] + ".png");
        if (*counter >= captures.size()) {
            I18n::instance().setLanguage(originalLanguage);
            step->stop();
            step->deleteLater();
            qApp->quit();
            return;
        }
        if (*counter < 7) {
            openPage(*counter);
        } else if (*counter < 10) {
            openPage(6);
            settings->findChild<QTabWidget *>()->setCurrentIndex(*counter - 6);
        } else if (*counter < 12) {
            openPage(0);
            UiKit::applyTheme(*counter == 10, *counter == 11);
        } else {
            UiKit::applyTheme();
            I18n::instance().setLanguage("en_US");
            openPage(*counter == 12 ? 0 : 6);
            if (*counter == 13)
                settings->findChild<QTabWidget *>()->setCurrentIndex(0);
        }
        ++*counter;
    });
    step->start(500);
}
