#include "settingwindow.h"
#include "Core/gnssservice.h"
#include "Core/i18n.h"
#include "Core/systemstatus.h"
#include "Core/timeservice.h"
#include "Core/wifiservice.h"
#include "Ui/uikit.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QSettings>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QTabWidget>
#include <QVBoxLayout>
SettingWindow::SettingWindow(GnssService *service, QWidget *parent)
    : QMainWindow(parent), gnss(service), wifi(new WifiService(this)),
      clock(new TimeService(this)) {
    setObjectName("page");
    UiKit::setAccent(this, "settings");
    auto *page = new QWidget;
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(8, 4, 8, 8);
    root->setSpacing(14);
    setCentralWidget(page);
    auto *heading = new QHBoxLayout;
    heading->addWidget(UiKit::iconLabel("settings", "settings", 34, true));
    heading->addWidget(UiKit::label(QStringLiteral("设置中心"), "title"));
    heading->addStretch();
    heading->addWidget(UiKit::iconLabel("language", "settings", 24));
    heading->addWidget(UiKit::label(QStringLiteral("界面语言"), "muted"));
    auto *chinese = UiKit::button(QStringLiteral("简体中文"));
    auto *english = UiKit::button("English");
    chinese->setObjectName("languageChinese");
    english->setObjectName("languageEnglish");
    chinese->setCheckable(true);
    english->setCheckable(true);
    auto *languages = new QButtonGroup(this);
    languages->addButton(chinese);
    languages->addButton(english);
    heading->addWidget(chinese);
    heading->addWidget(english);
    auto showLanguage = [chinese, english]() {
        chinese->setChecked(I18n::instance().language() == "zh_CN");
        english->setChecked(I18n::instance().language() == "en_US");
    };
    showLanguage();
    connect(chinese, &QPushButton::clicked, this, []() { I18n::instance().setLanguage("zh_CN"); });
    connect(english, &QPushButton::clicked, this, []() { I18n::instance().setLanguage("en_US"); });
    connect(&I18n::instance(), &I18n::languageChanged, this, showLanguage);
    root->addLayout(heading);
    auto *tabs = new QTabWidget;
    root->addWidget(tabs, 1);
    auto *networkPage = new QWidget;
    UiKit::setAccent(networkPage, "network");
    auto *networkLayout = new QVBoxLayout(networkPage);
    networkLayout->setContentsMargins(12, 16, 12, 8);
    auto *wifiRow = new QHBoxLayout;
    onButton = UiKit::button(QStringLiteral("开启 Wi-Fi"), "wifi");
    offButton = UiKit::button(QStringLiteral("关闭"), "power");
    UiKit::setAccent(offButton, "home");
    scanButton = UiKit::button(QStringLiteral("扫描网络"), "refresh");
    wifiRow->addWidget(onButton);
    wifiRow->addWidget(offButton);
    wifiRow->addStretch();
    wifiRow->addWidget(scanButton);
    networkLayout->addLayout(wifiRow);
    networks = new QListWidget;
    networkLayout->addWidget(networks, 1);
    auto *authRow = new QHBoxLayout;
    password = new QLineEdit;
    password->setObjectName("wifiPassword");
    password->setEchoMode(QLineEdit::Password);
    I18n::setProperty(password, "placeholderText", QStringLiteral("所选网络的密码"));
    connectButton = UiKit::button(QStringLiteral("连接网络"), "wifi");
    UiKit::setPrimary(connectButton, "network");
    authRow->addWidget(password, 1);
    authRow->addWidget(connectButton);
    networkLayout->addLayout(authRow);
    wifiStatus = UiKit::label(QString(), "muted");
    networkLayout->addWidget(wifiStatus);
    I18n::addTab(tabs, networkPage, QStringLiteral("网络连接"));
    auto *displayPage = new QWidget;
    UiKit::setAccent(displayPage, "display");
    auto *displayLayout = new QVBoxLayout(displayPage);
    displayLayout->setContentsMargins(16, 18, 16, 12);
    displayLayout->setSpacing(14);
    auto *night = new QCheckBox;
    I18n::setText(night, QStringLiteral("夜间模式"));
    UiKit::setButtonIcon(night, "moon", "video", 26, true);
    night->setChecked(QSettings().value("display/night", false).toBool());
    auto *large = new QCheckBox;
    I18n::setText(large, QStringLiteral("大字模式"));
    UiKit::setButtonIcon(large, "text", "settings", 26, true);
    large->setChecked(QSettings().value("display/large", false).toBool());
    displayLayout->addWidget(night);
    displayLayout->addWidget(large);
    auto *themeInfo = UiKit::label(QStringLiteral("显示偏好会自动保存，下次启动时恢复。"), "muted");
    displayLayout->addWidget(themeInfo);
    auto *volumeRow = new QHBoxLayout;
    volumeRow->addWidget(UiKit::iconLabel("volume", "music", 26, true));
    volumeRow->addWidget(UiKit::label(QStringLiteral("媒体音量")));
    volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    UiKit::setAccent(volumeSlider, "music");
    volumeSlider->setValue(QSettings().value("audio/volume", 35).toInt());
    volumeRow->addWidget(volumeSlider, 1);
    displayLayout->addLayout(volumeRow);
    auto *brightnessRow = new QHBoxLayout;
    brightnessRow->addWidget(UiKit::iconLabel("weather", "weather", 26, true));
    brightnessRow->addWidget(UiKit::label(QStringLiteral("屏幕亮度")));
    auto *brightness = new QSlider(Qt::Horizontal);
    brightness->setRange(5, 100);
    brightnessRow->addWidget(brightness, 1);
    displayLayout->addLayout(brightnessRow);
    auto *brightnessStatus = UiKit::label(QString(), "muted");
    displayLayout->addWidget(brightnessStatus);
    QString backlight;
    const auto entries = QDir("/sys/class/backlight").entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (!entries.isEmpty())
        backlight = "/sys/class/backlight/" + entries.first();
    QFile maxFile(backlight + "/max_brightness"), currentFile(backlight + "/brightness");
    int maximum = 0;
    if (!backlight.isEmpty() && maxFile.open(QIODevice::ReadOnly) &&
        currentFile.open(QIODevice::ReadOnly)) {
        maximum = maxFile.readAll().trimmed().toInt();
        brightness->setValue(maximum ? currentFile.readAll().trimmed().toInt() * 100 / maximum
                                     : 100);
    }
    brightness->setEnabled(maximum > 0);
    I18n::setText(brightnessStatus, maximum > 0 ? QStringLiteral("调整设备背光")
                                                : QStringLiteral("当前设备未提供可调背光"));
    connect(brightness, &QSlider::sliderReleased, this,
            [brightness, brightnessStatus, backlight, maximum]() {
                QFile file(backlight + "/brightness");
                if (!file.open(QIODevice::WriteOnly) ||
                    file.write(QByteArray::number(qMax(1, maximum * brightness->value() / 100))) <
                        0)
                    I18n::setText(brightnessStatus, QStringLiteral("亮度设置失败，请检查背光权限"));
                else
                    I18n::setText(brightnessStatus,
                                  QStringLiteral("屏幕亮度 %1%").arg(brightness->value()));
            });
    auto theme = [night, large]() {
        QSettings s;
        s.setValue("display/night", night->isChecked());
        s.setValue("display/large", large->isChecked());
        UiKit::applyTheme(night->isChecked(), large->isChecked());
    };
    connect(night, &QCheckBox::toggled, this, [theme](bool) { theme(); });
    connect(large, &QCheckBox::toggled, this, [theme](bool) { theme(); });
    connect(volumeSlider, &QSlider::valueChanged, this, &SettingWindow::volumeRequested);
    displayLayout->addStretch();
    I18n::addTab(tabs, displayPage, QStringLiteral("显示与声音"));
    auto *devicePage = new QWidget;
    UiKit::setAccent(devicePage, "camera");
    auto *deviceLayout = new QVBoxLayout(devicePage);
    deviceLayout->setContentsMargins(16, 18, 16, 12);
    deviceLayout->setSpacing(16);
    auto *positionRow = new QHBoxLayout;
    gnssStatus = UiKit::label(gnss->status());
    auto *gnssToggle = new QCheckBox;
    I18n::setText(gnssToggle, QStringLiteral("开启 GNSS 定位"));
    UiKit::setButtonIcon(gnssToggle, "pin", "map", 26, true);
    gnssToggle->setChecked(gnss->isEnabled());
    positionRow->addWidget(gnssToggle);
    positionRow->addStretch();
    positionRow->addWidget(gnssStatus);
    deviceLayout->addLayout(positionRow);
    connect(gnssToggle, &QCheckBox::toggled, gnss, &GnssService::setEnabled);
    connect(gnss, &GnssService::updated, this, [this, gnssToggle]() {
        I18n::setText(gnssStatus, gnss->status());
        QSignalBlocker block(gnssToggle);
        gnssToggle->setChecked(gnss->isEnabled());
    });
    memory = UiKit::label(QString());
    storage = UiKit::label(QString());
    uptime = UiKit::label(QString());
    deviceLayout->addWidget(UiKit::label(QStringLiteral("设备运行状态"), "heading"));
    auto *memoryRow = new QHBoxLayout;
    memoryRow->addWidget(UiKit::iconLabel("chip", "video", 26, true));
    memoryRow->addWidget(memory, 1);
    deviceLayout->addLayout(memoryRow);
    auto *storageRow = new QHBoxLayout;
    storageRow->addWidget(UiKit::iconLabel("storage", "settings", 26, true));
    storageRow->addWidget(storage, 1);
    deviceLayout->addLayout(storageRow);
    auto *uptimeRow = new QHBoxLayout;
    uptimeRow->addWidget(UiKit::iconLabel("clock", "weather", 26, true));
    uptimeRow->addWidget(uptime, 1);
    deviceLayout->addLayout(uptimeRow);
    deviceLayout->addStretch();
    deviceLayout->addWidget(UiKit::label(
        QStringLiteral("目标平台  NXP i.MX6ULL   ·   显示  1024 × 600   ·   Qt Widgets"), "muted"));
    I18n::addTab(tabs, devicePage, QStringLiteral("设备状态"));
    auto *timePage = new QWidget;
    UiKit::setAccent(timePage, "clock");
    auto *timeLayout = new QVBoxLayout(timePage);
    timeLayout->setContentsMargins(16, 18, 16, 12);
    timeLayout->addWidget(UiKit::label(QStringLiteral("日期与时间"), "heading"));
    auto *datetime = new QDateTimeEdit(QDateTime::currentDateTime());
    datetime->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    datetime->setCalendarPopup(true);
    timeLayout->addWidget(datetime);
    auto *timeRow = new QHBoxLayout;
    applyButton = UiKit::button(QStringLiteral("应用手动时间"), "clock");
    syncButton = UiKit::button(QStringLiteral("联网校时"), "refresh");
    UiKit::setPrimary(syncButton, "clock");
    timeRow->addWidget(applyButton);
    timeRow->addWidget(syncButton);
    timeRow->addStretch();
    timeLayout->addLayout(timeRow);
    timeStatus = UiKit::label(QStringLiteral("时间按系统时区显示，硬件时钟以 UTC 写入"), "muted");
    timeStatus->setWordWrap(true);
    timeLayout->addWidget(timeStatus);
    timeLayout->addStretch();
    I18n::addTab(tabs, timePage, QStringLiteral("日期时间"));
    UiKit::setTabIcon(tabs, 0, "wifi", "network");
    UiKit::setTabIcon(tabs, 1, "weather", "display");
    UiKit::setTabIcon(tabs, 2, "chip", "camera");
    UiKit::setTabIcon(tabs, 3, "clock", "clock");
    UiKit::setAccent(tabs, "network");
    connect(tabs, &QTabWidget::currentChanged, this, [tabs](int index) {
        const QStringList accents = {"network", "display", "camera", "clock"};
        UiKit::setAccent(tabs, accents.value(index, "settings"));
    });
    connect(applyButton, &QPushButton::clicked, this,
            [this, datetime]() { clock->setTime(datetime->dateTime()); });
    connect(syncButton, &QPushButton::clicked, clock, &TimeService::sync);
    connect(clock, &TimeService::status, this, [this](QString text, bool busy) {
        I18n::setText(timeStatus, text);
        applyButton->setEnabled(!busy);
        syncButton->setEnabled(!busy);
    });
    connect(onButton, &QPushButton::clicked, this, [this]() { wifi->setPowered(true); });
    connect(offButton, &QPushButton::clicked, this, [this]() { wifi->setPowered(false); });
    connect(scanButton, &QPushButton::clicked, wifi, &WifiService::scan);
    connect(connectButton, &QPushButton::clicked, this, [this]() {
        auto *item = networks->currentItem();
        if (!item) {
            I18n::setText(wifiStatus, QStringLiteral("请先选择一个网络"));
            return;
        }
        wifi->connectNetwork(item->data(Qt::UserRole).toString(), password->text());
        password->clear();
    });
    connect(wifi, &WifiService::updated, this, &SettingWindow::renderWifi);
    connect(&I18n::instance(), &I18n::languageChanged, this, &SettingWindow::renderWifi);
    refresh.setInterval(5000);
    connect(&refresh, &QTimer::timeout, this, &SettingWindow::renderSystem);
    renderWifi();
    renderSystem();
    UiKit::refreshIcons(this);
}
void SettingWindow::setVolume(int value) {
    QSignalBlocker block(volumeSlider);
    volumeSlider->setValue(value);
}
void SettingWindow::renderWifi() {
    I18n::setText(wifiStatus, wifi->status());
    for (auto *button : {scanButton, connectButton, onButton, offButton})
        button->setEnabled(!wifi->busy());
    if (wifi->busy())
        return;
    const QString selected = networks->currentItem()
                                 ? networks->currentItem()->data(Qt::UserRole).toString()
                                 : QString();
    networks->clear();
    for (const auto &network : wifi->networks()) {
        const bool online = network.state == "online" || network.state == "ready";
        auto *item = new QListWidgetItem(
            QString("%1    %2%  %3")
                .arg(network.name.isEmpty() ? I18n::instance().text(QStringLiteral("隐藏网络"))
                                            : network.name)
                .arg(network.strength)
                .arg(online ? I18n::instance().text(QStringLiteral("已连接")) : QString()),
            networks);
        item->setData(Qt::UserRole, network.path);
        if (network.path == selected)
            networks->setCurrentItem(item);
    }
}
void SettingWindow::renderSystem() {
    I18n::setText(memory, QStringLiteral("内存    %1").arg(SystemStatus::memory()));
    I18n::setText(storage, QStringLiteral("存储    %1").arg(SystemStatus::storage()));
    I18n::setText(uptime, QStringLiteral("运行时间    %1").arg(SystemStatus::uptime()));
}
void SettingWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    renderSystem();
    refresh.start();
}
void SettingWindow::hideEvent(QHideEvent *event) {
    refresh.stop();
    QMainWindow::hideEvent(event);
}
