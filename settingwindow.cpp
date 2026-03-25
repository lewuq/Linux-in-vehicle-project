#include "settingwindow.h"
#include "ui_settingwindow.h"
#include <QSet>
#include <QVBoxLayout>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QUrlQuery>
#include <QPixmap>
#include <QAbstractButton>

static QString sanitizeProcessError(const QString &raw)
{
    const QStringList lines = raw.split('\n');
    QStringList kept;
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i).trimmed();
        if (line.isEmpty()) {
            continue;
        }

        if (line.contains("GLib-CRITICAL", Qt::CaseInsensitive)
                || line.contains("Source ID", Qt::CaseInsensitive)
                || line.contains("not found when attempting to remove it", Qt::CaseInsensitive)
                || line.contains("This plugin does not support setting window masks", Qt::CaseInsensitive)
                || line.contains("No carrier", Qt::CaseInsensitive)) {
            continue;
        }

        kept.append(line);
    }
    return kept.join("\n");
}

SettingWindow::SettingWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SettingWindow)
{
    ui->setupUi(this);
    this->resize(1024,600);

    this->setStyleSheet(
        "QMainWindow#SettingWindow{background:#eef3f7;}"
        "QToolBox::tab{min-height:0px;max-height:0px;height:0px;padding:0px;margin:0px;border:none;background:transparent;color:transparent;}"
        "QToolBox::tab:selected{min-height:0px;max-height:0px;height:0px;padding:0px;margin:0px;border:none;background:transparent;color:transparent;}"
        "QLabel{color:#1f2d3c;font-size:17px;}"
        "QLabel#label_setting_time,QLabel#label_setting_date{background:#ffffff;border:1px solid rgba(0,0,0,22);border-radius:10px;padding:8px;font-size:22px;font-weight:700;}"
        "QPushButton{background:#ffffff;border:1px solid rgba(0,0,0,24);border-radius:12px;padding:10px 14px;font-size:18px;font-weight:700;color:#1f2d3c;}"
        "QPushButton:pressed{background:#e6eef7;}"
        "QListWidget,QLineEdit,QDateEdit,QSpinBox{background:#ffffff;border:1px solid rgba(0,0,0,20);border-radius:10px;padding:8px;font-size:17px;}"
    );

    ui->label_ShowGif->setFixedSize(500,300);
    ui->label_ShowGif->setScaledContents(true);
    ui->label_setting_date->setFixedWidth(225);
    ui->lineEdit_password->setPlaceholderText(QString::fromUtf8("输入 WiFi 密码"));

    wifiStatusLabel = new QLabel(QString::fromUtf8("WiFi 状态：未检测"), this);
    memoryInfoLabel = new QLabel(QString::fromUtf8("内存信息加载中..."), this);
    memoryInfoLabel->setWordWrap(true);
    memoryInfoLabel->setMinimumHeight(64);

    ui->verticalLayout_4->insertWidget(0, wifiStatusLabel);

    // 首页改成小米车机风设置项：纵向横条，点进去后切换到子模块。
    QWidget *moduleHome = new QWidget(this);
    QVBoxLayout *homeLayout = new QVBoxLayout(moduleHome);
    homeLayout->setContentsMargins(12, 12, 12, 12);
    homeLayout->setSpacing(12);

    auto makeModuleBtn = [this](const QString &title, const QString &desc) {
        QPushButton *btn = new QPushButton(QString::fromUtf8("%1\n%2").arg(title, desc), this);
        btn->setMinimumHeight(74);
        btn->setStyleSheet("QPushButton{background:#f6fbff;border:1px solid rgba(22,60,95,0.25);"
                           "border-radius:14px;padding:12px 16px;text-align:left;font-size:18px;font-weight:700;color:#18324a;}"
                           "QPushButton:pressed{background:#e9f2fb;}");
        return btn;
    };

    QPushButton *btnTimeModule = makeModuleBtn(QString::fromUtf8("时间与日期"), QString::fromUtf8("设置系统时间、日期"));
    QPushButton *btnWifiModule = makeModuleBtn(QString::fromUtf8("WiFi 网络"), QString::fromUtf8("扫描并连接 2.4G 无线网络"));
    QPushButton *btnMemoryModule = makeModuleBtn(QString::fromUtf8("内存状态"), QString::fromUtf8("查看内存占用与可用容量"));
    QPushButton *btnGnssModule = makeModuleBtn(QString::fromUtf8("GNSS 定位"), QString::fromUtf8("读取 EC20 定位并反查大致地址"));

    homeLayout->addWidget(btnTimeModule);
    homeLayout->addWidget(btnWifiModule);
    homeLayout->addWidget(btnMemoryModule);
    homeLayout->addWidget(btnGnssModule);
    homeLayout->addStretch(1);

    QWidget *memoryPage = new QWidget(this);
    QVBoxLayout *memoryLayout = new QVBoxLayout(memoryPage);
    memoryLayout->setContentsMargins(12, 12, 12, 12);
    memoryLayout->setSpacing(10);
    QLabel *memoryPageTitle = new QLabel(QString::fromUtf8("内存状态"), this);
    memoryPageTitle->setStyleSheet("font-size:22px;font-weight:700;color:#16324a;");
    QPushButton *memoryRefreshBtnPage = new QPushButton(QString::fromUtf8("刷新内存信息"), this);
    memoryLayout->addWidget(memoryPageTitle);
    memoryLayout->addWidget(memoryInfoLabel);
    memoryLayout->addWidget(memoryRefreshBtnPage, 0, Qt::AlignLeft);
    memoryLayout->addStretch(1);

    QWidget *gnssPage = new QWidget(this);
    gnssPageWidget = gnssPage;
    QVBoxLayout *gnssLayout = new QVBoxLayout(gnssPage);
    gnssLayout->setContentsMargins(0, 0, 0, 0);
    gnssLayout->setSpacing(0);
    QLabel *gnssTitle = new QLabel(QString::fromUtf8("GNSS 定位"), this);
    gnssTitle->setStyleSheet("font-size:22px;font-weight:700;color:#16324a;");
    gnssStatusLabel = new QLabel(QString::fromUtf8("状态: 未读取"), this);
    gnssCoordLabel = new QLabel(QString::fromUtf8("经纬度: --"), this);
    gnssLocationLabel = new QLabel(QString::fromUtf8("位置: 惠州惠城区惠州学院"), this);
    gnssRawLabel = new QLabel(QString::fromUtf8("NMEA: --"), this);
    gnssRawLabel->setWordWrap(true);
    gnssMapLabel = new QLabel(this);
    gnssMapLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    gnssMapLabel->setAlignment(Qt::AlignCenter);
    gnssMapLabel->setStyleSheet("QLabel{background:#ffffff;border:1px solid rgba(0,0,0,20);border-radius:12px;color:#5a6a78;}");
    gnssMapLabel->setText(QString::fromUtf8("地图加载中..."));

    QPushButton *btnGnssStart = new QPushButton(QString::fromUtf8("开启 GNSS"), this);
    QPushButton *btnGnssStop = new QPushButton(QString::fromUtf8("关闭 GNSS"), this);
    QPushButton *btnGnssRefresh = new QPushButton(QString::fromUtf8("获取位置"), this);
    QHBoxLayout *gnssBtnLayout = new QHBoxLayout;
    gnssBtnLayout->addWidget(btnGnssStart);
    gnssBtnLayout->addWidget(btnGnssStop);
    gnssBtnLayout->addWidget(btnGnssRefresh);

    gnssMapLabel->setMinimumHeight(300);
    gnssMapLabel->setMaximumHeight(300);

    gnssLayout->addWidget(gnssMapLabel, 0);
    gnssLayout->addWidget(gnssTitle, 0);
    gnssLayout->addWidget(gnssStatusLabel, 0);
    gnssLayout->addWidget(gnssCoordLabel, 0);
    gnssLayout->addWidget(gnssLocationLabel, 0);
    gnssLayout->addLayout(gnssBtnLayout, 0);
    gnssLayout->addWidget(gnssRawLabel, 1);

    const int oldSetTimeIndex = ui->toolBox->indexOf(ui->setTime);
    const int oldSetNetIndex = ui->toolBox->indexOf(ui->setNet);
    ui->toolBox->insertItem(0, moduleHome, QString::fromUtf8("设置首页"));
    ui->toolBox->setItemText(oldSetTimeIndex + 1, QString::fromUtf8("时间与日期"));
    ui->toolBox->setItemText(oldSetNetIndex + 1, QString::fromUtf8("WiFi 网络"));
    const int memoryPageIndex = ui->toolBox->addItem(memoryPage, QString::fromUtf8("内存状态"));
    const int gnssPageIndex = ui->toolBox->addItem(gnssPage, QString::fromUtf8("GNSS 定位"));

    connect(btnTimeModule, &QPushButton::clicked, this, [this]() {
        ui->toolBox->setCurrentWidget(ui->setTime);
    });
    connect(btnWifiModule, &QPushButton::clicked, this, [this]() {
        ui->toolBox->setCurrentWidget(ui->setNet);
    });
    connect(btnMemoryModule, &QPushButton::clicked, this, [this, memoryPage]() {
        refreshMemoryInfo();
        ui->toolBox->setCurrentWidget(memoryPage);
    });
    connect(btnGnssModule, &QPushButton::clicked, this, [this, gnssPage]() {
        ui->toolBox->setCurrentWidget(gnssPage);
    });
    connect(memoryRefreshBtnPage, &QPushButton::clicked, this, [this]() {
        refreshMemoryInfo();
    });
    connect(btnGnssStart, &QPushButton::clicked, this, [this]() {
        QString error;
        runCommand("sh", QStringList() << "-c" << "printf 'AT+QGPS=1\\r\\n' >/dev/ttyUSB2", &error);
        if (!error.isEmpty()) {
            if (gnssStatusLabel) {
                gnssStatusLabel->setText(QString::fromUtf8("状态: 开启失败"));
            }
            return;
        }
        if (gnssStatusLabel) {
            gnssStatusLabel->setText(QString::fromUtf8("状态: GNSS 已开启"));
        }
    });
    connect(btnGnssStop, &QPushButton::clicked, this, [this]() {
        QString error;
        runCommand("sh", QStringList() << "-c" << "printf 'AT+QGPSEND\\r\\n' >/dev/ttyUSB2", &error);
        if (gnssStatusLabel) {
            gnssStatusLabel->setText(error.isEmpty() ? QString::fromUtf8("状态: GNSS 已关闭") : QString::fromUtf8("状态: 关闭失败"));
        }
    });
    connect(btnGnssRefresh, &QPushButton::clicked, this, [this]() {
        refreshGnssInfo();
    });
    Q_UNUSED(memoryPageIndex);
    Q_UNUSED(gnssPageIndex);

    QPushButton *btnWifiOff = new QPushButton(QString::fromUtf8("关闭WiFi"), this);
    btnWifiOff->setMinimumSize(0, 50);
    btnWifiOff->setStyleSheet("QPushButton{background:#fff4f4;border:1px solid rgba(120,0,0,0.25);border-radius:12px;color:#6d1f1f;font-size:17px;font-weight:700;padding:10px 14px;}"
                             "QPushButton:pressed{background:#f6e4e4;}");
    ui->horizontalLayout_4->insertWidget(1, btnWifiOff);
    connect(btnWifiOff, &QPushButton::clicked, this, [this]() {
        QString error;
        runCommand("ifconfig", QStringList() << "wlan0" << "down", &error);
        QString connmanErr;
        runCommand("connmanctl", QStringList() << "disable" << "wifi", &connmanErr);

        if (!error.trimmed().isEmpty()) {
            if (wifiStatusLabel) {
                wifiStatusLabel->setText(QString::fromUtf8("WiFi 状态：关闭失败"));
            }
            QMessageBox::warning(this, QString::fromUtf8("错误"), error);
            return;
        }

        ui->listWidget->clear();
        wifiServiceMap.clear();
        if (wifiStatusLabel) {
            wifiStatusLabel->setText(QString::fromUtf8("WiFi 状态：已关闭"));
        }
        if (!connmanErr.trimmed().isEmpty()) {
            qDebug() << "connman disable wifi warning:" << connmanErr;
        }
    });

    // 右侧区域只保留返回，隐藏旧版大面积 GIF 面板。
    ui->horizontalLayout_7->setContentsMargins(8, 8, 8, 8);
    ui->horizontalLayout_7->setStretch(0, 1);
    ui->horizontalLayout_7->setStretch(1, 0);
    ui->label_ShowGif->hide();
    ui->pBtn_PauseGif->hide();
    ui->pBtn_SwitchGif->hide();
    ui->pushButton_8->hide();

    // 只压缩 QToolBox 自身的 tab 按钮高度，避免多页 tab 累积导致顶部留白。
    const QList<QAbstractButton *> toolboxTabButtons = ui->toolBox->findChildren<QAbstractButton *>(QString(), Qt::FindDirectChildrenOnly);
    for (int i = 0; i < toolboxTabButtons.size(); ++i) {
        QAbstractButton *tabBtn = toolboxTabButtons.at(i);
        tabBtn->setVisible(false);
        tabBtn->setMinimumHeight(0);
        tabBtn->setMaximumHeight(0);
        tabBtn->setFixedHeight(0);
    }

    QPushButton *topRightBackBtn = new QPushButton(QString::fromUtf8("返回"), ui->centralwidget);
    topRightBackBtn->setObjectName("topRightBackBtn");
    topRightBackBtn->setFixedSize(92, 44);
    topRightBackBtn->move(this->width() - 120, 18);
    topRightBackBtn->setStyleSheet("QPushButton{background:#ffffff;border:1px solid rgba(0,0,0,28);border-radius:12px;font-size:18px;font-weight:700;color:#1f2d3c;padding:4px 12px;}"
                                 "QPushButton:pressed{background:#e8eef6;}");
    connect(topRightBackBtn, &QPushButton::clicked, this, [this]() {
        if (ui->toolBox->currentIndex() != 0) {
            ui->toolBox->setCurrentIndex(0);
            return;
        }
        on_pushButton_8_clicked();
    });

    ui->toolBox->setCurrentIndex(0);

    timer = new QTimer;
    timer->setInterval(500);
    timer->start();
    connect(timer,SIGNAL(timeout()),this,SLOT(on_timer_updateTime()));
    // 仍保留 GIF 逻辑函数，但默认不展示其面板，避免设置页冗余。
    refreshMemoryInfo();
    QTimer::singleShot(0, this, [this]() {
        updateGnssMapPreview(QString::fromUtf8("惠州惠城区惠州学院"));
    });

}

SettingWindow::~SettingWindow()
{
    delete ui;
}

bool SettingWindow::applySystemDateTime(const QString &dateTimeText)
{
    process.start("date", QStringList() << "-s" << dateTimeText);
    process.waitForFinished();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qDebug() << process.readAllStandardError();
        process.close();
        return false;
    }

    process.start("hwclock", QStringList() << "-w");
    process.waitForFinished();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qDebug() << process.readAllStandardError();
        process.close();
        return false;
    }

    qDebug() << process.readAllStandardOutput();
    process.close();
    return true;
}

void SettingWindow::showTimeSettingPanel()
{
    ui->toolBox->setCurrentIndex(0);
    this->show();
    this->raise();
    this->activateWindow();
}

void SettingWindow::on_pushButton_8_clicked()
{
    this->hide();
}


/* 遍历本地gif文件  */
void SettingWindow::ScanGif()
{
    QDir dir(QCoreApplication::applicationDirPath() + LocalGifPath);
    QDir dirAbsolutePath(dir.absolutePath());
    if(dirAbsolutePath.exists())
    {
        QStringList filter;
        filter<<"*.gif";
        QFileInfoList files = dirAbsolutePath.entryInfoList(filter,QDir::Files);
        GifSum = files.count();
        gif_Files.clear();
        for(int i=0;i<files.count();i++)
        {
            gif_Files.append(files.at(i).absoluteFilePath());
        }
    }
    if(0==GifSum)
    {
        ui->label_ShowGif->setText("./MyGif 目录下没有可用的Gif文件");
        return;
    }
    currentGifIndex = 0;
    movie->setFileName(gif_Files.at(0));
    ui->label_ShowGif->setMovie(movie);
    movie->start();
}

/* 暂停或者播放 gif 图片的显示  */
void SettingWindow::on_pBtn_PauseGif_clicked(bool checked)
{
    if(0==GifSum)return;
    if(!checked)
    {
        ui->pBtn_PauseGif->setText("暂停");
        movie->start();
    }
    else
    {
        ui->pBtn_PauseGif->setText("播放");
        movie->stop();
    }
}

/*切换gif图片  */
void SettingWindow::on_pBtn_SwitchGif_clicked()
{
    if(GifSum==0)return;
    int id = qrand()%GifSum;
    movie->stop();
    movie->setFileName(gif_Files.at(id));

    if(!ui->pBtn_ModifyTime->isChecked())
        movie->start();
}

/* 修改日期按钮 */
void SettingWindow::on_pBtn_ModifyDate_clicked()
{
    QString dateStr = ui->dateEdit->date().toString("yyyy-MM-dd");
    QString timeStr = QTime::currentTime().toString("hh:mm:ss");
    QString string = QString("%1 %2").arg(dateStr).arg(timeStr);
    qDebug()<<string;
    if (!applySystemDateTime(string)) {
        ui->label_setting_time->setText("设置失败");
    }
}


/* 修改时间按钮 */
void SettingWindow::on_pBtn_ModifyTime_clicked()
{
    int hour = ui->spinBox_hour->value();
    int min = ui->spinBox_Min->value();
    int second = ui->spinBox_Sec->value();
    QString string = QString("%1:%2:%3")
            .arg(hour, 2, 10, QChar('0'))
            .arg(min, 2, 10, QChar('0'))
            .arg(second, 2, 10, QChar('0'));
    qDebug()<<string;
    if (!applySystemDateTime(string)) {
        ui->label_setting_time->setText("设置失败");
    }

}

/* 更新当前窗口的时间显示 */
void SettingWindow::on_timer_updateTime()
{
    ui->label_setting_date->setText(QDate::currentDate().toString("yyyy-MM-dd"));
    ui->label_setting_time->setText(QTime::currentTime().toString("hh:mm:ss"));

    QPushButton *topRightBackBtn = ui->centralwidget->findChild<QPushButton *>("topRightBackBtn");
    if (topRightBackBtn) {
        topRightBackBtn->move(this->width() - topRightBackBtn->width() - 24, 12);
        topRightBackBtn->raise();
    }
}

QString SettingWindow::runCommand(const QString &program, const QStringList &arguments, QString *errorOut)
{
    process.start(program, arguments);
    process.waitForFinished();
    const QString error = sanitizeProcessError(QString::fromLocal8Bit(process.readAllStandardError()));
    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    process.close();
    if (errorOut) {
        *errorOut = error;
    }
    return output;
}

QString SettingWindow::readGnssNmeaSnapshot()
{
    QString error;
    const QString output = runCommand("sh", QStringList() << "-c" << "timeout 2 cat /dev/ttyUSB1", &error);
    if (!error.isEmpty() && !error.contains("Terminated", Qt::CaseInsensitive)) {
        return QString();
    }
    return output;
}

double SettingWindow::nmeaToDecimal(const QString &value, bool isLatitude, const QString &hemisphere) const
{
    bool ok = false;
    const double raw = value.toDouble(&ok);
    if (!ok || raw <= 0.0) {
        return 0.0;
    }

    const int degreeDigits = isLatitude ? 2 : 3;
    if (value.size() < degreeDigits) {
        return 0.0;
    }

    const double degrees = value.left(degreeDigits).toDouble(&ok);
    if (!ok) {
        return 0.0;
    }

    const double minutes = value.mid(degreeDigits).toDouble(&ok);
    if (!ok) {
        return 0.0;
    }

    double decimal = degrees + minutes / 60.0;
    if (hemisphere == "S" || hemisphere == "W") {
        decimal = -decimal;
    }
    return decimal;
}

bool SettingWindow::parseCoordinatesFromNmea(const QString &nmeaText, double *lat, double *lon) const
{
    if (!lat || !lon) {
        return false;
    }

    const QStringList lines = nmeaText.split('\n', QString::SkipEmptyParts);
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i).trimmed();
        if (line.startsWith("$GPRMC") || line.startsWith("$GNRMC")) {
            const QStringList f = line.split(',');
            if (f.size() > 6 && f.at(2) == "A") {
                const double lt = nmeaToDecimal(f.at(3), true, f.at(4));
                const double ln = nmeaToDecimal(f.at(5), false, f.at(6));
                if (lt != 0.0 && ln != 0.0) {
                    *lat = lt;
                    *lon = ln;
                    return true;
                }
            }
        }
    }

    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i).trimmed();
        if (line.startsWith("$GPGGA") || line.startsWith("$GNGGA")) {
            const QStringList f = line.split(',');
            if (f.size() > 6 && f.at(6).toInt() > 0) {
                const double lt = nmeaToDecimal(f.at(2), true, f.at(3));
                const double ln = nmeaToDecimal(f.at(4), false, f.at(5));
                if (lt != 0.0 && ln != 0.0) {
                    *lat = lt;
                    *lon = ln;
                    return true;
                }
            }
        }
    }

    return false;
}

QString SettingWindow::reverseGeocode(double lat, double lon)
{
    QUrl url("https://nominatim.openstreetmap.org/reverse");
    QUrlQuery query;
    query.addQueryItem("format", "jsonv2");
    query.addQueryItem("lat", QString::number(lat, 'f', 6));
    query.addQueryItem("lon", QString::number(lon, 'f', 6));
    query.addQueryItem("accept-language", "zh-CN");
    url.setQuery(query);

    QNetworkAccessManager mgr;
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "VehicleTerminal/1.0");
    QNetworkReply *reply = mgr.get(req);

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(&timeout, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    timeout.start(3500);
    loop.exec();

    QString result;
    if (timeout.isActive() && reply->error() == QNetworkReply::NoError) {
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            const QJsonObject obj = doc.object();
            result = obj.value("display_name").toString().trimmed();
        }
    }

    reply->deleteLater();
    return result;
}

QString SettingWindow::baiduStaticMapUrl(const QString &center, const QSize &size) const
{
    // 复用 Map 模块中的百度 AK
    const QString ak = "eza5Cxk2cUWZirqGXVHVsSjAkRLaF60j";
    QUrl url("http://api.map.baidu.com/staticimage/v2");
    QUrlQuery query;
    Q_UNUSED(size);
    const int width = 1024;
    const int height = 300;
    query.addQueryItem("ak", ak);
    query.addQueryItem("width", QString::number(width));
    query.addQueryItem("height", QString::number(height));
    query.addQueryItem("scale", "1");
    query.addQueryItem("copyright", "1");
    query.addQueryItem("center", center);
    query.addQueryItem("markers", center);
    query.addQueryItem("zoom", "15");
    query.addQueryItem("markerStyles", "l,A");
    url.setQuery(query);
    return url.toString();
}

void SettingWindow::updateGnssMapPreview(const QString &center)
{
    if (!gnssMapLabel) {
        return;
    }

    QNetworkAccessManager mgr;
    const QSize requestSize(1024, 300);
    QNetworkRequest req(QUrl(baiduStaticMapUrl(center, requestSize)));
    QNetworkReply *reply = mgr.get(req);

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(&timeout, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    timeout.start(3500);
    loop.exec();

    QPixmap pix;
    bool ok = false;
    if (timeout.isActive() && reply->error() == QNetworkReply::NoError) {
        const QByteArray data = reply->readAll();
        ok = pix.loadFromData(data);
    }
    reply->deleteLater();

    if (ok) {
        gnssMapLabel->setText(QString());
        gnssMapLabel->setPixmap(pix.scaled(gnssMapLabel->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    } else {
        gnssMapLabel->setPixmap(QPixmap());
        gnssMapLabel->setText(QString::fromUtf8("地图加载失败，当前位置：%1").arg(center));
    }
}

void SettingWindow::refreshGnssInfo()
{
    const QString fallbackLocation = QString::fromUtf8("惠州惠城区惠州学院");
    const QString nmea = readGnssNmeaSnapshot();
    if (nmea.trimmed().isEmpty()) {
        if (gnssStatusLabel) gnssStatusLabel->setText(QString::fromUtf8("状态: 未读取到 GNSS 数据"));
        if (gnssCoordLabel) gnssCoordLabel->setText(QString::fromUtf8("经纬度: --"));
        if (gnssLocationLabel) gnssLocationLabel->setText(QString::fromUtf8("位置: %1").arg(fallbackLocation));
        if (gnssRawLabel) gnssRawLabel->setText(QString::fromUtf8("NMEA: --"));
        updateGnssMapPreview(fallbackLocation);
        return;
    }

    if (gnssRawLabel) {
        const QString firstLine = nmea.split('\n', QString::SkipEmptyParts).value(0).trimmed();
        gnssRawLabel->setText(QString::fromUtf8("NMEA: %1").arg(firstLine));
    }

    double lat = 0.0;
    double lon = 0.0;
    if (!parseCoordinatesFromNmea(nmea, &lat, &lon)) {
        if (gnssStatusLabel) gnssStatusLabel->setText(QString::fromUtf8("状态: 已读取但未定位成功"));
        if (gnssCoordLabel) gnssCoordLabel->setText(QString::fromUtf8("经纬度: --"));
        if (gnssLocationLabel) gnssLocationLabel->setText(QString::fromUtf8("位置: %1").arg(fallbackLocation));
        updateGnssMapPreview(fallbackLocation);
        return;
    }

    if (gnssStatusLabel) gnssStatusLabel->setText(QString::fromUtf8("状态: 定位成功"));
    if (gnssCoordLabel) gnssCoordLabel->setText(QString::fromUtf8("经纬度: %1, %2")
                                                .arg(QString::number(lat, 'f', 6),
                                                     QString::number(lon, 'f', 6)));

    QString location = reverseGeocode(lat, lon);
    if (location.isEmpty()) {
        location = fallbackLocation;
    }
    if (gnssLocationLabel) {
        gnssLocationLabel->setText(QString::fromUtf8("位置: %1").arg(location));
    }

    const QString centerByCoord = QString("%1,%2")
            .arg(QString::number(lon, 'f', 6), QString::number(lat, 'f', 6));
    updateGnssMapPreview(centerByCoord);
}

void SettingWindow::refreshMemoryInfo()
{
    QFile memFile("/proc/meminfo");
    if (!memFile.open(QIODevice::ReadOnly)) {
        if (memoryInfoLabel) {
            memoryInfoLabel->setText(QString::fromUtf8("内存信息读取失败"));
        }
        return;
    }

    QHash<QString, qint64> memValues;
    while (!memFile.atEnd()) {
        const QByteArray line = memFile.readLine().trimmed();
        const int colonPos = line.indexOf(':');
        if (colonPos <= 0) {
            continue;
        }

        const QString key = QString::fromLatin1(line.left(colonPos));
        const QString rest = QString::fromLatin1(line.mid(colonPos + 1)).simplified();
        bool ok = false;
        const qint64 value = rest.split(' ').value(0).toLongLong(&ok);
        if (ok) {
            memValues.insert(key, value);
        }
    }
    memFile.close();

    const qint64 totalKb = memValues.value("MemTotal", 0);
    qint64 availableKb = memValues.value("MemAvailable", -1);
    if (availableKb < 0) {
        // 老内核没有 MemAvailable 时的估算方案。
        availableKb = memValues.value("MemFree", 0)
                + memValues.value("Buffers", 0)
                + memValues.value("Cached", 0)
                + memValues.value("SReclaimable", 0)
                - memValues.value("Shmem", 0);
    }

    if (totalKb <= 0) {
        if (memoryInfoLabel) {
            memoryInfoLabel->setText(QString::fromUtf8("内存信息不可用"));
        }
        return;
    }

    availableKb = qMax<qint64>(0, qMin(totalKb, availableKb));
    const qint64 usedKb = totalKb - availableKb;
    const double totalMb = totalKb / 1024.0;
    const double usedMb = usedKb / 1024.0;
    const double freeMb = availableKb / 1024.0;
    const double usage = (usedMb / qMax(1.0, totalMb)) * 100.0;

    if (memoryInfoLabel) {
        memoryInfoLabel->setText(QString::fromUtf8("内存总量: %1 MB\n已使用: %2 MB\n可用: %3 MB\n占用率: %4%").arg(QString::number(totalMb, 'f', 1),
                                                                                                      QString::number(usedMb, 'f', 1),
                                                                                                      QString::number(freeMb, 'f', 1),
                                                                                                      QString::number(usage, 'f', 1)));
    }
}

/* 连接wifi 功能 不再使用*/
void SettingWindow::on_pushButton_3_clicked()
{
    QString error;
    runCommand("ifconfig", QStringList() << "wlan0" << "up", &error);
    if (error.contains("rfkill", Qt::CaseInsensitive)) {
        runCommand("rfkill", QStringList() << "unblock" << "all", nullptr);
        error.clear();
        runCommand("ifconfig", QStringList() << "wlan0" << "up", &error);
    }
    if(0<error.length())
    {
        if (wifiStatusLabel) {
            wifiStatusLabel->setText(QString::fromUtf8("WiFi 状态：开启失败"));
        }
        QMessageBox::warning(this,"error",error);
        return;
    }

    QString connmanError;
    runCommand("connmanctl", QStringList() << "enable" << "wifi", &connmanError);
    runCommand("connmanctl", QStringList() << "agent" << "on", nullptr);
    if (!connmanError.isEmpty()) {
        if (wifiStatusLabel) {
            wifiStatusLabel->setText(QString::fromUtf8("WiFi 状态：connman 启用失败"));
        }
        QMessageBox::warning(this, "error", connmanError);
        return;
    }

    if (wifiStatusLabel) {
        wifiStatusLabel->setText(QString::fromUtf8("WiFi 状态：wlan0 已开启，connman 已启用"));
    }
}

/* 连接wifi 功能 不再使用*/
void SettingWindow::on_pushButton_4_clicked()
{
    QString error;
    runCommand("connmanctl", QStringList() << "scan" << "wifi", &error);
    if(!error.isEmpty())
    {
        QMessageBox::warning(this,"error",error);
        if (wifiStatusLabel) {
            wifiStatusLabel->setText(QString::fromUtf8("WiFi 状态：扫描失败"));
        }
        return;
    }

    const QString output = runCommand("connmanctl", QStringList() << "services", &error);
    if(!error.isEmpty()) {
        QMessageBox::warning(this,"error",error);
        return;
    }

    QStringList scanResult = output.split("\n");
    ui->listWidget->clear();
    wifiServiceMap.clear();
    QSet<QString> seenSsid;
    for (int i=0; i<scanResult.count();i++ ) {
        QString line = scanResult.at(i).trimmed();
        if (line.isEmpty()) {
            continue;
        }

        while (!line.isEmpty() && !line.at(0).isLetterOrNumber() && line.at(0) != 'w') {
            line.remove(0, 1);
        }

        const int idPos = line.indexOf(" wifi_");
        if (idPos <= 0) {
            continue;
        }

        const QString ssid = line.left(idPos).trimmed();
        const QString serviceId = line.mid(idPos + 1).trimmed();
        if (!serviceId.startsWith("wifi_")) {
            continue;
        }

        const QString displayName = ssid.isEmpty() ? serviceId : ssid;
        if (seenSsid.contains(displayName)) {
            continue;
        }
        seenSsid.insert(displayName);
        wifiServiceMap.insert(displayName, serviceId);
        ui->listWidget->addItem(displayName);
        qDebug() << "wifi:" << displayName << serviceId;
    }

    if (wifiStatusLabel) {
        wifiStatusLabel->setText(QString::fromUtf8("WiFi 状态：扫描完成，发现 %1 个网络").arg(ui->listWidget->count()));
    }
}

/* 连接wifi 功能 不再使用*/
void SettingWindow::on_pushButton_5_clicked()
{
    if (ui->listWidget->selectedItems().isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择一个 WiFi"));
        return;
    }
    const QString wifiName = ui->listWidget->selectedItems().first()->text();
    const QString serviceId = wifiServiceMap.value(wifiName);
    if (serviceId.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("未找到对应服务 ID，请先重新扫描"));
        return;
    }
    QString password = ui->lineEdit_password->text();
    qDebug() << "wifiName:" << wifiName << "password:" << password;

    QString error;
    // connmanctl 的 connect 可能依赖已保存配置；若首次连接，尝试经 connmanctl config 保存密码。
    if (!password.trimmed().isEmpty()) {
        runCommand("connmanctl", QStringList() << "config" << serviceId << "--passphrase" << password, nullptr);
    }
    const QString connectOutput = runCommand("connmanctl", QStringList() << "connect" << serviceId, &error);
    const bool connectFailed = (!error.trimmed().isEmpty())
            || connectOutput.contains("Error", Qt::CaseInsensitive)
            || connectOutput.contains("fail", Qt::CaseInsensitive);
    bool connected = !connectFailed;

    if (!connected && !password.trimmed().isEmpty()) {
        auto shellEscapeSingleQuotes = [](QString text) {
            return text.replace("'", "'\\\"'\\\"'");
        };
        const QString sidEscaped = shellEscapeSingleQuotes(serviceId);
        const QString pwdEscaped = shellEscapeSingleQuotes(password);
        const QString script = QString("printf 'agent on\\nconnect %1\\n%2\\nquit\\n' | connmanctl")
                .arg(sidEscaped, pwdEscaped);

        QString fallbackErr;
        const QString fallbackOut = runCommand("sh", QStringList() << "-c" << script, &fallbackErr);
        connected = fallbackErr.trimmed().isEmpty()
                && !fallbackOut.contains("Error", Qt::CaseInsensitive)
                && !fallbackOut.contains("fail", Qt::CaseInsensitive);
        if (!connected) {
            if (fallbackErr.trimmed().isEmpty()) {
                error = fallbackOut;
            } else {
                error = fallbackErr;
            }
        }
    }

    if(!connected)
    {
        const QString details = (error.trimmed().isEmpty() ? connectOutput : error);
        QMessageBox::warning(this,"error",details);
        if (wifiStatusLabel) {
            wifiStatusLabel->setText(QString::fromUtf8("WiFi 状态：连接失败"));
        }
        return;
    }
    QMessageBox::information(this,"Wifi Connected","wifi 链接成功");
    if (wifiStatusLabel) {
        wifiStatusLabel->setText(QString::fromUtf8("WiFi 状态：已连接 %1").arg(wifiName));
    }
}

///* 城市选择改变时触发 */
//void SettingWindow::on_comboBox_city_currentTextChanged(const QString &arg1)
//{
//    if(weatherWindow != nullptr)
//    {
//        weatherWindow->setCity(arg1);
//    }
//}
