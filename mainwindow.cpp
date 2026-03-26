#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>
#include <QProcess>
#include <QPainter>
#include <QPainterPath>
#include <QLabel>
#include <QRegularExpression>

static QString normalizeSpeechText(const QString &input)
{
    QString text = input;
    text = text.simplified();
    text.remove(QRegularExpression(QString::fromUtf8(R"([，。！？；：,.!?;:"'`~@#$%^&*()_+=\[\]{}<>|/\\-])")));
    text.remove(' ');
    return text;
}

static bool containsAnyPhrase(const QString &text, const QStringList &phrases)
{
    for (int i = 0; i < phrases.size(); ++i) {
        if (text.contains(phrases.at(i))) {
            return true;
        }
    }
    return false;
}

static QIcon buildCircularCardIcon(const QString &path, int diameter)
{
    QPixmap src(path);
    if (src.isNull()) {
        return QIcon();
    }

    QPixmap canvas(diameter, diameter);
    canvas.fill(Qt::transparent);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QRect circleRect(2, 2, diameter - 4, diameter - 4);

    // Soft circle plate with light rim to avoid dark edge on LCD.
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 230));
    painter.drawEllipse(circleRect);

    QPainterPath clipPath;
    clipPath.addEllipse(circleRect.adjusted(3, 3, -3, -3));
    painter.setClipPath(clipPath);

    QPixmap scaled = src.scaled(circleRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int x = circleRect.center().x() - scaled.width() / 2;
    int y = circleRect.center().y() - scaled.height() / 2;
    painter.drawPixmap(x, y, scaled);

    painter.setClipping(false);
    painter.setPen(QPen(QColor(215, 225, 236, 200), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(circleRect.adjusted(1, 1, -1, -1));

    return QIcon(canvas);
}

static QIcon buildVideoModuleIcon(int diameter)
{
    QPixmap canvas(diameter, diameter);
    canvas.fill(Qt::transparent);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QRect outerRect(2, 2, diameter - 4, diameter - 4);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 235));
    painter.drawEllipse(outerRect);

    QRect filmRect = outerRect.adjusted(24, 34, -24, -34);
    painter.setBrush(QColor(38, 62, 89, 230));
    painter.setPen(QPen(QColor(205, 220, 235, 210), 1));
    painter.drawRoundedRect(filmRect, 16, 16);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(208, 226, 246, 220));
    const int notchW = 10;
    const int notchH = 7;
    for (int i = 0; i < 4; ++i) {
        const int x = filmRect.left() + 12 + i * ((filmRect.width() - 24) / 3);
        painter.drawRoundedRect(QRect(x, filmRect.top() + 6, notchW, notchH), 2, 2);
        painter.drawRoundedRect(QRect(x, filmRect.bottom() - 12, notchW, notchH), 2, 2);
    }

    QPainterPath playPath;
    const QPoint c = filmRect.center();
    playPath.moveTo(c.x() - 8, c.y() - 18);
    playPath.lineTo(c.x() + 20, c.y());
    playPath.lineTo(c.x() - 8, c.y() + 18);
    playPath.closeSubpath();
    painter.setBrush(QColor(96, 205, 255, 235));
    painter.drawPath(playPath);

    painter.setPen(QPen(QColor(215, 225, 236, 200), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(outerRect.adjusted(1, 1, -1, -1));

    return QIcon(canvas);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // ===== 工程约束：窗口装饰控制（补充配置） =====
    // 主.pro和main.cpp中已设置全屏/固定尺寸，此处为额外防护
    this->setFocusPolicy(Qt::StrongFocus);  // 确保主窗口能接收所有键盘事件
    
    QWidget *clockCard = new QWidget(this);
    clockCard->setObjectName("clockCard");
    QVBoxLayout *clockCardLayout = new QVBoxLayout(clockCard);
    clockCardLayout->setContentsMargins(10, 10, 10, 10);
    clockCardLayout->addWidget(&clock);
    ui->videoVLayout->addWidget(clockCard);

    this->resize(1024,600);

    // 主页面视觉升级：小米车机感浅色背景 + 六张悬浮卡片（4功能 + 时钟 + 信息）。
    ui->gridLayout->setContentsMargins(18, 18, 18, 18);
    ui->gridLayout->setHorizontalSpacing(16);
    ui->gridLayout->setVerticalSpacing(16);

    ui->pBtn_Weather->setStyleSheet("");
    ui->pBtn_Music->setStyleSheet("");
    ui->pBtn_Map->setStyleSheet("");
    ui->pBtn_Monitor->setStyleSheet("");
    ui->pushButton_2->setStyleSheet("");
    ui->pBtn_Setting->setStyleSheet("");
    ui->groupBox->setStyleSheet("");
    ui->label_Date->setStyleSheet("");

    const int iconDiameter = 188;
    ui->pBtn_Weather->setIcon(buildCircularCardIcon(":/img/weather.jpg", iconDiameter));
    ui->pBtn_Music->setIcon(buildCircularCardIcon(":/img/music.png", iconDiameter));
    ui->pBtn_Map->setIcon(buildCircularCardIcon(":/img/map.jpg", iconDiameter));
    ui->pBtn_Monitor->setIcon(buildCircularCardIcon(":/img/monitor.png", iconDiameter));
    ui->pushButton_2->setIcon(buildVideoModuleIcon(iconDiameter));
    ui->pBtn_Setting->setIcon(buildCircularCardIcon(":/img/settting.png", iconDiameter));
    ui->pBtn_Weather->setIconSize(QSize(iconDiameter, iconDiameter));
    ui->pBtn_Music->setIconSize(QSize(iconDiameter, iconDiameter));
    ui->pBtn_Map->setIconSize(QSize(iconDiameter, iconDiameter));
    ui->pBtn_Monitor->setIconSize(QSize(iconDiameter, iconDiameter));
    ui->pushButton_2->setIconSize(QSize(iconDiameter, iconDiameter));
    ui->pBtn_Setting->setIconSize(QSize(iconDiameter, iconDiameter));
    ui->pBtn_Weather->setText(QString());
    ui->pBtn_Music->setText(QString());
    ui->pBtn_Map->setText(QString());
    ui->pBtn_Monitor->setText(QString());
    ui->pushButton_2->setText(QString());
    ui->pBtn_Setting->setText(QString());

    auto addBottomCaption = [](QPushButton *btn, const QString &text) {
        QVBoxLayout *captionLayout = new QVBoxLayout(btn);
        captionLayout->setContentsMargins(6, 6, 6, 8);
        captionLayout->addStretch();
        QLabel *caption = new QLabel(text, btn);
        caption->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        caption->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        caption->setStyleSheet("color:#1f3447;font-size:19px;font-weight:700;background:transparent;");
        captionLayout->addWidget(caption, 0, Qt::AlignHCenter | Qt::AlignBottom);
    };
    addBottomCaption(ui->pBtn_Weather, QString::fromUtf8("天气"));
    addBottomCaption(ui->pBtn_Music, QString::fromUtf8("音乐"));
    addBottomCaption(ui->pBtn_Map, QString::fromUtf8("地图"));
    addBottomCaption(ui->pBtn_Monitor, QString::fromUtf8("监控"));
    addBottomCaption(ui->pushButton_2, QString::fromUtf8("视频"));
    addBottomCaption(ui->pBtn_Setting, QString::fromUtf8("设置"));

    ui->pushButton_3->hide();
    ui->pushButton_4->hide();
    ui->pushButton_5->hide();
    ui->pushButton_6->hide();

    this->setStyleSheet(R"(
        QMainWindow#MainWindow {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #d8dde2, stop:0.55 #e3e8ec, stop:1 #edf1f4);
        }

        QWidget#centralwidget,
        QWidget#page {
            background: transparent;
        }

        QWidget#clockCard,
        QGroupBox#groupBox,
        QPushButton#pBtn_Weather,
        QPushButton#pBtn_Music,
        QPushButton#pBtn_Map,
        QPushButton#pBtn_Monitor,
        QPushButton#pushButton_2,
        QPushButton#pBtn_Setting {
            background-color: rgba(178, 209, 240, 0.98);
            border: 1px solid rgba(1, 9, 17, 0.24);
            border-radius: 24px;
        }

        QWidget#clockCard {
            padding: 6px;
        }

        QPushButton#pBtn_Weather,
        QPushButton#pBtn_Music,
        QPushButton#pBtn_Map,
        QPushButton#pBtn_Monitor,
        QPushButton#pushButton_2,
        QPushButton#pBtn_Setting {
            padding: 6px 10px 6px 10px;
            text-align: bottom center;
        }

        QPushButton#pBtn_Weather:pressed,
        QPushButton#pBtn_Music:pressed,
        QPushButton#pBtn_Map:pressed,
        QPushButton#pBtn_Monitor:pressed,
        QPushButton#pushButton_2:pressed,
        QPushButton#pBtn_Setting:pressed {
            background-color: rgba(212, 223, 233, 0.98);
        }

        QGroupBox#groupBox {
            margin-top: 0px;
            padding: 14px;
        }

        QLabel#label_Time {
            color: #3b4d5d;
            font-size: 20px;
            font-weight: 700;
        }

        QLabel#label_Date {
            color: #1d2936;
            font-size: 28px;
            font-weight: 700;
        }

        QLabel#label_temp {
            color: #8a3f3f;
            font-size: 22px;
            font-weight: 700;
        }

        QLabel#label_humi {
            color: #2f6e6a;
            font-size: 22px;
            font-weight: 700;
        }
    )");

    // 首页保留一个主时钟（左侧仪表），右侧仅显示日期与功能入口，避免重复显示两个时钟。
    ui->label_Time->setText("系统日期");

    quickSetTimeBtn = new QPushButton(QString::fromUtf8("手动校时"), ui->groupBox);
    quickSetTimeBtn->setObjectName("quickSetTimeBtn");
    quickSetTimeBtn->setMinimumSize(180, 56);
    quickSetTimeBtn->setStyleSheet("QPushButton#quickSetTimeBtn{background:#d8ecff;color:#13406e;"
                                   "background:#e3eaf1;color:#2a4358;"
                                   "border:1px solid #b8c6d3;border-radius:14px;font-size:22px;"
                                   "font-weight:700;padding:10px 16px;}"
                                   "QPushButton#quickSetTimeBtn:pressed{background:#d8e2ea;}");
    ui->verticalLayout->addWidget(quickSetTimeBtn);
    connect(quickSetTimeBtn, SIGNAL(clicked()), this, SLOT(on_quickSetTime_clicked()));

    networkSyncBtn = new QPushButton(QString::fromUtf8("联网校时(北京时间)"), ui->groupBox);
    networkSyncBtn->setObjectName("networkSyncBtn");
    networkSyncBtn->setMinimumSize(180, 48);
    networkSyncBtn->setStyleSheet("QPushButton#networkSyncBtn{background:#dde8f2;color:#294458;"
                                  "border:1px solid #b8c6d3;border-radius:12px;font-size:18px;"
                                  "font-weight:700;padding:8px 14px;}"
                                  "QPushButton#networkSyncBtn:pressed{background:#cfdde9;}");
    ui->verticalLayout->addWidget(networkSyncBtn);
    connect(networkSyncBtn, SIGNAL(clicked()), this, SLOT(on_networkTimeSync_clicked()));

    ui->stackedWidget->installEventFilter(this);
    dht11 = new Dht11;

    // ===== 工程约束：子窗口动态创建与内存管理 =====
    // 【重要】以下子窗口现在采用 new 动态分配，而非栈分配
    // 好处：1) 除非被显示，否则不占用内存；2) 支持懒加载；3) 便于后续优化成单例模式
    baiduMap = new BaiduMap(this);      // 地图模块
    monitor = Monitor::getInstance();    // 监测模块（已使用单例）
    musicPlayer = new MusicPlayer(this); // 音乐模块
    weather = new Weather(this);         // 天气模块
    
    // 窗口列表：用于集中管理和事件分发
    windows.append(baiduMap);
    windows.append(monitor);
    windows.append(weather);
    windows.append(musicPlayer);
    time = new QTimer;
    time->setInterval(500);
    time->start();
    dht11->start();
    request  = new QNetworkRequest;
    networkManage = new QNetworkAccessManager;
    request->setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/json"));
    connect(networkManage,SIGNAL(finished(QNetworkReply *)),this,SLOT(getSpeechResult(QNetworkReply *)));
    AsrThread = new SpeechRecognition();
    AsrThread->startSpeechRecognition();
    connect(AsrThread,SIGNAL(RecordFinished()),this,SLOT(on_handleRecord()));
    connect(time,SIGNAL(timeout()),this,SLOT(on_timer_updateTime()));
    connect(dht11,SIGNAL(updateDht11Data(QString ,QString )),this,SLOT(on_update_humidity_temp(QString, QString)));

    connect(this,SIGNAL(SendCommandToMap(int)),baiduMap,SLOT(on_handleCommand(int)));
    connect(this,SIGNAL(SendCommandToMonitor(int)),monitor,SLOT(on_handleCommand(int)));
    connect(this,SIGNAL(SendCommandToMusic(int)),musicPlayer,SLOT(on_handleCommand(int)));
    connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(on_pBtn_Video_clicked()));

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pBtn_Setting_clicked()
{
    settingWindow.showTimeSettingPanel();
}

void MainWindow::on_quickSetTime_clicked()
{
    settingWindow.showTimeSettingPanel();
}

void MainWindow::on_update_humidity_temp(QString humidity, QString temp)
{
    ui->label_humi->setText(humidity);
    ui->label_temp->setText(temp);
}

void MainWindow::getSpeechResult(QNetworkReply *reply)
{
    qDebug()<<"getSpeechResult";
    if (!reply) {
        qDebug() << "reply is null";
        return;
    }

    qDebug()<<"reply is Readable"<<reply->isReadable();

    QByteArray content = reply->readAll();
    const int lastNewline = content.lastIndexOf('\n');
    if (lastNewline >= 0) {
        content.remove(lastNewline, 1);
    }
    qDebug()<<content;
    QJsonDocument doc = QJsonDocument::fromJson(content);
    if(!doc.isObject()){
        qDebug()<<"Netjson not an jsonObject!";
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    const int errNo = obj.value("err_no").toInt(0);
    const QString errMsg = obj.value("err_msg").toString();
    if (errNo != 0) {
        qDebug() << "ASR error:" << errNo << errMsg;
        reply->deleteLater();
        return;
    }

    QJsonArray resutls = obj.value("result").toArray();
    if (resutls.isEmpty()) {
        qDebug() << "ASR result empty";
        reply->deleteLater();
        return;
    }

    QString AsrResult = resutls.at(0).toString();
    qDebug()<<"识别结果:"<<AsrResult;

    const QString cmd = normalizeSpeechText(AsrResult);

    if (containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("播放音乐") << QString::fromUtf8("打开音乐") << QString::fromUtf8("进入音乐") << QString::fromUtf8("来点音乐") << QString::fromUtf8("听歌"))) {
        musicPlayer->show();
        musicPlayer->raise();
        musicPlayer->activateWindow();
        musicPlayer->playRandomSong();
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("下一首") << QString::fromUtf8("下一曲") << QString::fromUtf8("切歌") << QString::fromUtf8("换一首")))
    {
        emit SendCommandToMusic(MUSIC_COMMAND_SHOW);
        emit SendCommandToMusic(MUSIC_COMMAND_NEXT);
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("上一首") << QString::fromUtf8("上一曲") << QString::fromUtf8("返回上一首")))
    {
        emit SendCommandToMusic(MUSIC_COMMAND_SHOW);
        emit SendCommandToMusic(MUSIC_COMMAND_PRE);
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("暂停音乐") << QString::fromUtf8("暂停播放") << QString::fromUtf8("停止音乐")))
    {
        emit SendCommandToMusic(MUSIC_COMMAND_SHOW);
        emit SendCommandToMusic(MUSIC_COMMAND_PAUSE);
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("退出音乐") << QString::fromUtf8("关闭音乐")))
    {
        emit SendCommandToMusic(MUSIC_COMMAND_CLOSE);
        this->show();
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("打开地图") << QString::fromUtf8("进入地图") << QString::fromUtf8("查看地图")))
    {
        emit SendCommandToMap(MAP_COMMAND_SHOW);
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("地图放大") << QString::fromUtf8("放大地图") << QString::fromUtf8("放大一点") << QString::fromUtf8("放大")))
    {
        emit SendCommandToMap(MAP_COMMAND_SHOW);
        emit SendCommandToMap(MAP_COMMAND_AMPLIFY);

    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("地图缩小") << QString::fromUtf8("缩小地图") << QString::fromUtf8("缩小一点") << QString::fromUtf8("缩小")))
    {
        emit SendCommandToMap(MAP_COMMAND_SHOW);
        emit SendCommandToMap(MAP_COMMAND_SHRINK);

    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("开始倒车") << QString::fromUtf8("打开监控") << QString::fromUtf8("打开摄像头") << QString::fromUtf8("查看监控") << QString::fromUtf8("倒车影像")))
    {
        monitor->show();
        monitor->raise();
        monitor->activateWindow();
        monitor->myStart();
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("查看天气") << QString::fromUtf8("天气怎么样") << QString::fromUtf8("打开天气")))
    {
        weather->show();
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("播放视频") << QString::fromUtf8("打开视频") << QString::fromUtf8("进入视频")))
    {
        oneVideo.ScanLocalVideos();
        oneVideo.show();
        oneVideo.raise();
        oneVideo.activateWindow();
        oneVideo.onPlayRandomVideo();
    }
    else if(cmd.contains(QString::fromUtf8("下一条视频")) || cmd.contains(QString::fromUtf8("下一个视频")))
    {
        oneVideo.onNextVideo();
    }
    else if(cmd.contains(QString::fromUtf8("上一条视频")) || cmd.contains(QString::fromUtf8("上一个视频")))
    {
        oneVideo.onPrevVideo();
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("暂停视频") << QString::fromUtf8("播放视频") << QString::fromUtf8("继续播放") << QString::fromUtf8("视频播放") << QString::fromUtf8("视频暂停")))
    {
        oneVideo.onPlayPauseVideo();
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("打开设置") << QString::fromUtf8("系统设置")))
    {
        settingWindow.showTimeSettingPanel();
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("关闭地图") << QString::fromUtf8("退出地图")))
    {
        emit SendCommandToMap(MAP_COMMAND_CLOSE);
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("关闭监控") << QString::fromUtf8("退出监控") << QString::fromUtf8("停止监控")))
    {
        emit SendCommandToMonitor(MONITOR_COMMAND_CLOSE);
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("关闭视频") << QString::fromUtf8("退出视频")))
    {
        oneVideo.hide();
        this->show();
    }
    else if(containsAnyPhrase(cmd, QStringList() << QString::fromUtf8("关闭设置") << QString::fromUtf8("退出设置")))
    {
        settingWindow.hide();
        this->show();
    }
    else
    {
        qDebug()<<"未识别的指令:"<<AsrResult;
    }
    
    // ===== 工程约束：网络数据及时释放 =====
    // 【关键】解析完 JSON 后立刻释放 QNetworkReply 对象
    // 防止长期占用缓冲区，导致 512MB 内存设备 OOM
    reply->deleteLater();
}
QPoint point,last_point;//按下坐标

bool MainWindow::eventFilter(QObject *watched, QEvent *ev)
{

    int index,sum;
        QMouseEvent* event = static_cast<QMouseEvent*>(ev);
        index = ui->stackedWidget->currentIndex();
        sum = ui->stackedWidget->count();
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            qDebug()<<"mouse press";
            point.setY(event->globalY());     // 记录按下点的y坐标
            point.setX(event->globalX());     // 记录按下点的x坐标
            break;
        case QEvent::MouseButtonRelease:
            qDebug()<<"mouse release";
            last_point.setY(event->globalY());     // 记录按下点的y坐标
            last_point.setX(event->globalX());     // 记录按下点的x坐标

            if(last_point.x()-point.x()>200)
            {
                ui->stackedWidget->setCurrentIndex(index<(sum-1)?index+1:0);
            }
            else if(point.x()-last_point.x()>200)
            {
                ui->stackedWidget->setCurrentIndex(index>0?index-1:(sum-1));
            }
            break;
        default:
            break;
        }
    return QWidget::eventFilter(watched,event);//将事件传递给父类
}

void MainWindow::on_pBtn_Music_clicked()
{
    musicPlayer->show();
}

void MainWindow::on_pBtn_Video_clicked()
{
    oneVideo.ScanLocalVideos();
    oneVideo.show();
    oneVideo.raise();
    oneVideo.activateWindow();
}

void MainWindow::on_networkTimeSync_clicked()
{
    syncBeijingTime();
}

void MainWindow::syncBeijingTime()
{
    if (timeSyncManager == nullptr) {
        timeSyncManager = new QNetworkAccessManager(this);
        connect(timeSyncManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(on_timeSyncReply(QNetworkReply*)));
    }

    ui->label_Time->setText(QString::fromUtf8("北京时间同步中..."));
    timeSyncManager->get(QNetworkRequest(QUrl("http://quan.suning.com/getSysTime.do")));
}

void MainWindow::on_timeSyncReply(QNetworkReply *reply)
{
    const QByteArray content = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(content);

    if (!doc.isObject()) {
        ui->label_Time->setText(QString::fromUtf8("校时失败(网络)"));
        reply->deleteLater();
        return;
    }

    const QString beijingTime = doc.object().value("sysTime2").toString();
    if (beijingTime.isEmpty()) {
        ui->label_Time->setText(QString::fromUtf8("校时失败(数据)"));
        reply->deleteLater();
        return;
    }

    QProcess process;
    process.start("date", QStringList() << "-s" << beijingTime);
    process.waitForFinished();
    bool ok = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);

    if (ok) {
        process.start("hwclock", QStringList() << "-w");
        process.waitForFinished();
        ok = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    }

    ui->label_Time->setText(ok ? QString::fromUtf8("北京时间已同步") : QString::fromUtf8("校时失败(权限)"));
    if (ok) {
        on_timer_updateTime();
    }

    reply->deleteLater();
}

void MainWindow::on_pBtn_Weather_clicked()
{
    weather->show();
}

void MainWindow::on_pBtn_Monitor_clicked()
{
    monitor->show();
    monitor->myStart();
}

void MainWindow::on_pBtn_Map_clicked()
{
    baiduMap->show();
}

void MainWindow::on_timer_updateTime()
{
    QString date = QDate::currentDate().toString("yyyy-MM-dd");
    ui->label_Date->setText(date);
}

void MainWindow::on_handleRecord()
{
    qDebug()<<"TransWavToStr";
    QFile file("./record.wav");
    qDebug()<<file.exists();
    qDebug()<<file.isReadable();
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug()<<"无法打开文件";
        return ;
    }
    request->setUrl(QUrl::fromUserInput("http://vop.baidu.com/server_api"));
    QByteArray fileData = file.readAll();
    file.close();
    QByteArray base64Encoded = fileData.toBase64();
    if (fileData.isEmpty()) {
        qDebug() << "录音文件为空，跳过识别请求";
        return;
    }

    QJsonObject obj;
    obj.insert("format",QJsonValue("wav"));
    obj.insert("rate",QJsonValue(16000));
    obj.insert("channel",QJsonValue(1));
    obj.insert("dev_pid", QJsonValue(1537));
    obj.insert("cuid",QJsonValue("L5a9DNZMyQD4MyipDR3ck7jhmdvtagj2"));
    obj.insert("token",QJsonValue("25.e8ff34b13abf78f8173fca370bfc46d2.315360000.2089007905.282335-122390697"));
    obj.insert("speech",QJsonValue(QString::fromLatin1(base64Encoded)));
    obj.insert("len",QJsonValue(fileData.length()));
    QByteArray byte_array = QJsonDocument(obj).toJson();
    networkManage->post(*request,byte_array);

}
