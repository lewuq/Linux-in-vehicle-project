#include "onevideo.h"
#include "ui_oneVideo.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QCollator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCoreApplication>
#include <QPalette>
#include <QMessageBox>
#include <QRandomGenerator>

static QString resolveVideoDir(const QString &rawPath)
{
    if (QDir::isAbsolutePath(rawPath)) {
        return rawPath;
    }
    return QCoreApplication::applicationDirPath() + "/" + rawPath;
}

OneVideo::OneVideo(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::OneVideo),
      videoListWidget(nullptr),
      btnPrev(nullptr),
      btnPlayPause(nullptr),
      btnNext(nullptr),
      btnScan(nullptr),
    btnDelete(nullptr),
      btnClose(nullptr),
    btnVolDown(nullptr),
    btnVolUp(nullptr),
      titleLabel(nullptr),
    volumeLabel(nullptr),
    volumeSlider(nullptr),
            currentVideoIndex(-1)
{
    ui->setupUi(this);
    this->resize(1024, 600);
    this->setWindowTitle(QString::fromUtf8("本地视频"));

    const QList<QWidget *> widgets = this->findChildren<QWidget *>();
    for (QWidget *w : widgets) {
        if (w->objectName() == "verticalLayoutWidget") {
            w->hide();
        }
    }

    m_pPlayer = new QMediaPlayer(this);
    m_pPlayerWidget = new QVideoWidget(this);
    m_pPlayer->setVideoOutput(m_pPlayerWidget);

    titleLabel = new QLabel(QString::fromUtf8("本地视频播放器"), this);
    titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    btnScan = new QPushButton(QString::fromUtf8("扫描视频"), this);
    btnDelete = new QPushButton(QString::fromUtf8("删除视频"), this);
    btnClose = new QPushButton(QString::fromUtf8("返回"), this);

    btnVolDown = new QPushButton(QString::fromUtf8("音量-"), this);
    btnVolUp = new QPushButton(QString::fromUtf8("音量+"), this);
    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(20);
    volumeLabel = new QLabel(QString::fromUtf8("音量 20"), this);
    volumeLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    videoListWidget = new QListWidget(this);
    videoListWidget->setMinimumWidth(300);

    btnPrev = new QPushButton(this);
    btnPlayPause = new QPushButton(this);
    btnNext = new QPushButton(this);
    btnPrev->setMinimumSize(108, 56);
    btnPlayPause->setMinimumSize(116, 56);
    btnNext->setMinimumSize(108, 56);
    btnPrev->setText(QString::fromUtf8("上一条"));
    btnPlayPause->setText(QString::fromUtf8("播放"));
    btnNext->setText(QString::fromUtf8("下一条"));

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(titleLabel, 1);
    topLayout->addWidget(btnScan, 0);
    topLayout->addWidget(btnDelete, 0);
    topLayout->addWidget(btnClose, 0);

    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->addWidget(m_pPlayerWidget, 3);
    contentLayout->addWidget(videoListWidget, 2);

    QHBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->addStretch(1);
    controlLayout->addWidget(btnPrev);
    controlLayout->addSpacing(24);
    controlLayout->addWidget(btnPlayPause);
    controlLayout->addSpacing(24);
    controlLayout->addWidget(btnNext);
    controlLayout->addSpacing(18);
    controlLayout->addWidget(btnVolDown);
    controlLayout->addWidget(volumeSlider, 1);
    controlLayout->addWidget(btnVolUp);
    controlLayout->addWidget(volumeLabel);
    controlLayout->addStretch(1);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 16, 20, 16);
    rootLayout->setSpacing(12);
    rootLayout->addLayout(topLayout);
    rootLayout->addLayout(contentLayout, 1);
    rootLayout->addLayout(controlLayout);

    this->setStyleSheet(
        "QWidget#OneVideo{background:#eff3f7;}"
        "QLabel{color:#1f2c3a;font-size:24px;font-weight:700;}"
        "QVideoWidget{background:#000000;border:1px solid rgba(0,0,0,45);border-radius:12px;}"
        "QListWidget{background:#ffffff;border:1px solid rgba(0,0,0,28);border-radius:12px;font-size:18px;padding:6px;}"
        "QListWidget::item{height:36px;padding:4px 8px;}"
        "QListWidget::item:selected{background:#dfeefc;color:#0f3b62;border-radius:8px;}"
        "QPushButton{background:#ffffff;border:1px solid rgba(0,0,0,28);border-radius:12px;color:#1f2c3a;font-size:18px;font-weight:700;padding:8px 14px;}"
        "QPushButton:pressed{background:#e6eef7;}"
        "QSlider::groove:horizontal{height:7px;border-radius:3px;background:rgba(0,0,0,22);}"
        "QSlider::handle:horizontal{width:16px;margin:-5px 0;border-radius:8px;background:#2f88db;}"
    );

    btnPrev->setStyleSheet("QPushButton{background:#ffffff;border:1px solid rgba(0,0,0,28);border-radius:12px;padding:8px 12px;font-size:18px;font-weight:700;}QPushButton:pressed{background:#e6eef7;}");
    btnPlayPause->setStyleSheet("QPushButton{background:#ffffff;border:1px solid rgba(0,0,0,28);border-radius:12px;padding:8px 12px;font-size:18px;font-weight:700;}QPushButton:pressed{background:#e6eef7;}");
    btnNext->setStyleSheet("QPushButton{background:#ffffff;border:1px solid rgba(0,0,0,28);border-radius:12px;padding:8px 12px;font-size:18px;font-weight:700;}QPushButton:pressed{background:#e6eef7;}");

    QPixmap coverPix(":/video/images/btn_play1.png");
    if (!coverPix.isNull()) {
        const QPalette oldPalette = m_pPlayerWidget->palette();
        QPalette pal = oldPalette;
        pal.setBrush(QPalette::Window, QBrush(coverPix.scaled(220, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        m_pPlayerWidget->setAutoFillBackground(true);
        m_pPlayerWidget->setPalette(pal);
    }

    connect(btnPrev, SIGNAL(clicked()), this, SLOT(onPrevVideo()));
    connect(btnPlayPause, SIGNAL(clicked()), this, SLOT(onPlayPauseVideo()));
    connect(btnNext, SIGNAL(clicked()), this, SLOT(onNextVideo()));
    connect(btnScan, SIGNAL(clicked()), this, SLOT(onScanVideosClicked()));
    connect(btnDelete, SIGNAL(clicked()), this, SLOT(onDeleteVideoClicked()));
    connect(btnClose, SIGNAL(clicked()), this, SLOT(onCloseClicked()));
    connect(btnVolDown, SIGNAL(clicked()), this, SLOT(onVolumeDownClicked()));
    connect(btnVolUp, SIGNAL(clicked()), this, SLOT(onVolumeUpClicked()));
    connect(volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(onVolumeSliderChanged(int)));
    connect(videoListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(onVideoSelected(int)));
    connect(m_pPlayer, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(onPlayerStateChanged(QMediaPlayer::State)));

    m_pPlayer->setVolume(20);
    onVolumeSliderChanged(20);

    ScanLocalVideos();
}

OneVideo::~OneVideo()
{
    delete ui;
}

QStringList OneVideo::candidateVideoDirs() const
{
    QStringList dirs;
    dirs << resolveVideoDir("Video")
         << "/home/root/mytest/Qt/test1/Video"
         << "/home/root/mytest/Video"
         << "/run/media/mmcblk0p1"
         << "/run/media/mmcblk0p1/Video"
         << "/mnt/mmc0"
         << "/mnt/mmc0/Video"
         << "/media/sdcard/Video";
    return dirs;
}

void OneVideo::ScanLocalVideos()
{
    QStringList filters;
    filters << "*.mp4" << "*.MP4" << "*.avi" << "*.AVI" << "*.mkv" << "*.MKV"
            << "*.mov" << "*.MOV" << "*.flv" << "*.FLV";

    QSet<QString> seen;
    QStringList paths;
    for (int i = 0; i < candidateVideoDirs().size(); ++i) {
        const QString dirPath = candidateVideoDirs().at(i);
        QDir dir(dirPath);
        if (!dir.exists()) {
            continue;
        }
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
        for (int j = 0; j < files.size(); ++j) {
            const QString absolutePath = files.at(j).absoluteFilePath();
            const QString canonicalPath = QFileInfo(absolutePath).canonicalFilePath();
            const QString key = canonicalPath.isEmpty() ? absolutePath : canonicalPath;
            if (seen.contains(key)) {
                continue;
            }
            seen.insert(key);
            paths.append(absolutePath);
        }
    }

    QCollator collator(QLocale(QLocale::Chinese, QLocale::China));
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(paths.begin(), paths.end(), [&collator](const QString &a, const QString &b) {
        return collator.compare(QFileInfo(a).completeBaseName(), QFileInfo(b).completeBaseName()) < 0;
    });

    videoFiles = paths;
    videoListWidget->clear();
    for (int i = 0; i < videoFiles.size(); ++i) {
        videoListWidget->addItem(QFileInfo(videoFiles.at(i)).completeBaseName());
    }

    if (videoFiles.isEmpty()) {
        titleLabel->setText(QString::fromUtf8("未找到本地视频，请检查 Video/ 或 SD 卡路径"));
        m_pPlayer->stop();
        currentVideoIndex = -1;
        updatePlayButtonIcon();
        return;
    }

    if (currentVideoIndex < 0 || currentVideoIndex >= videoFiles.size()) {
        currentVideoIndex = 0;
    }
    videoListWidget->setCurrentRow(currentVideoIndex);
    titleLabel->setText(QFileInfo(videoFiles.at(currentVideoIndex)).fileName());
    m_pPlayer->stop();
    updatePlayButtonIcon();
}

void OneVideo::playVideoAt(int index)
{
    if (index < 0 || index >= videoFiles.size()) {
        return;
    }
    currentVideoIndex = index;
    const QString path = videoFiles.at(currentVideoIndex);
    m_pPlayer->setMedia(QUrl::fromLocalFile(path));
    titleLabel->setText(QFileInfo(path).fileName());
    updatePlayButtonIcon();
}

void OneVideo::updatePlayButtonIcon()
{
    if (m_pPlayer->state() == QMediaPlayer::PlayingState) {
        btnPlayPause->setText(QString::fromUtf8("暂停"));
    } else {
        btnPlayPause->setText(QString::fromUtf8("播放"));
    }
}

void OneVideo::PlayVideo()
{
    if (videoFiles.isEmpty()) {
        ScanLocalVideos();
    } else if (currentVideoIndex < 0) {
        playVideoAt(0);
    } else {
        m_pPlayer->play();
    }
    updatePlayButtonIcon();
}

void OneVideo::onPlayRandomVideo()
{
    if (videoFiles.isEmpty()) {
        ScanLocalVideos();
    }
    if (videoFiles.isEmpty()) {
        return;
    }

    const int randomIndex = QRandomGenerator::global()->bounded(videoFiles.size());
    videoListWidget->setCurrentRow(randomIndex);
    playVideoAt(randomIndex);
    m_pPlayer->play();
}

void OneVideo::onPrevVideo()
{
    if (videoFiles.isEmpty()) {
        return;
    }
    int index = currentVideoIndex;
    if (index < 0) {
        index = 0;
    }
    index = (index - 1 + videoFiles.size()) % videoFiles.size();
    videoListWidget->setCurrentRow(index);
    playVideoAt(index);
    m_pPlayer->play();
}

void OneVideo::onNextVideo()
{
    if (videoFiles.isEmpty()) {
        return;
    }
    int index = currentVideoIndex;
    if (index < 0) {
        index = -1;
    }
    index = (index + 1) % videoFiles.size();
    videoListWidget->setCurrentRow(index);
    playVideoAt(index);
    m_pPlayer->play();
}

void OneVideo::onPlayPauseVideo()
{
    if (videoFiles.isEmpty()) {
        ScanLocalVideos();
        return;
    }

    if (m_pPlayer->state() == QMediaPlayer::PlayingState) {
        m_pPlayer->pause();
    } else if (currentVideoIndex >= 0) {
        if (m_pPlayer->media().isNull()) {
            playVideoAt(currentVideoIndex);
        }
        m_pPlayer->play();
    } else {
        playVideoAt(0);
        m_pPlayer->play();
    }
    updatePlayButtonIcon();
}

void OneVideo::onVideoSelected(int row)
{
    if (row < 0 || row >= videoFiles.size()) {
        return;
    }
    currentVideoIndex = row;
    titleLabel->setText(QFileInfo(videoFiles.at(currentVideoIndex)).fileName());
    if (m_pPlayer->state() != QMediaPlayer::PlayingState) {
        m_pPlayer->setMedia(QUrl::fromLocalFile(videoFiles.at(currentVideoIndex)));
        m_pPlayer->stop();
        updatePlayButtonIcon();
    }
}

void OneVideo::onScanVideosClicked()
{
    ScanLocalVideos();
}

void OneVideo::onPlayerStateChanged(QMediaPlayer::State state)
{
    updatePlayButtonIcon();
    if (state == QMediaPlayer::StoppedState && !videoFiles.isEmpty() && currentVideoIndex >= 0) {
        if (m_pPlayer->position() >= m_pPlayer->duration() - 1000) {
            onNextVideo();
        }
    }
}

void OneVideo::onCloseClicked()
{
    this->hide();
}

void OneVideo::onDeleteVideoClicked()
{
    int index = videoListWidget->currentRow();
    if (index < 0 || index >= videoFiles.size()) {
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择要删除的视频"));
        return;
    }

    const QString filePath = videoFiles.at(index);
    const QString fileName = QFileInfo(filePath).fileName();
    const int ret = QMessageBox::question(this,
                                          QString::fromUtf8("确认删除"),
                                          QString::fromUtf8("确定删除视频？\n%1").arg(fileName),
                                          QMessageBox::Yes | QMessageBox::No,
                                          QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    m_pPlayer->stop();
    m_pPlayer->setMedia(QMediaContent());

    if (!QFile::remove(filePath)) {
        QMessageBox::warning(this, QString::fromUtf8("删除失败"), QString::fromUtf8("文件删除失败，请检查权限"));
        return;
    }

    QMessageBox::information(this, QString::fromUtf8("完成"), QString::fromUtf8("视频已删除"));
    currentVideoIndex = qMin(index, videoFiles.size() - 2);
    ScanLocalVideos();
}

void OneVideo::onVolumeSliderChanged(int value)
{
    const int volume = qBound(0, value, 100);
    if (m_pPlayer->volume() != volume) {
        m_pPlayer->setVolume(volume);
    }
    volumeLabel->setText(QString::fromUtf8("音量 %1").arg(volume));
}

void OneVideo::onVolumeDownClicked()
{
    volumeSlider->setValue(qMax(0, volumeSlider->value() - 10));
}

void OneVideo::onVolumeUpClicked()
{
    volumeSlider->setValue(qMin(100, volumeSlider->value() + 10));
}

