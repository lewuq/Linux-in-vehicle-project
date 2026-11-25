#include "onevideo.h"
#include "Core/i18n.h"
#include "Ui/uikit.h"
#include <QHBoxLayout>
#include <QListWidget>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>
#include <QVideoWidget>
static QString durationText(qint64 ms) {
    return QString("%1:%2")
        .arg(ms / 60000, 2, 10, QChar('0'))
        .arg((ms / 1000) % 60, 2, 10, QChar('0'));
}
OneVideo::OneVideo(QWidget *parent) : QWidget(parent), library(this) {
    setObjectName("page");
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 4, 8, 8);
    root->setSpacing(14);
    auto *header = new QHBoxLayout;
    header->addWidget(UiKit::label(QStringLiteral("本地影院"), "title"));
    header->addStretch();
    scanButton = UiKit::button(QStringLiteral("扫描视频"), "refresh");
    header->addWidget(scanButton);
    root->addLayout(header);
    auto *body = new QHBoxLayout;
    video = new QVideoWidget;
    video->setMinimumSize(320, 200);
    player.setVideoOutput(video);
    list = new QListWidget;
    list->setMinimumWidth(220);
    list->setMaximumWidth(280);
    body->addWidget(video, 3);
    body->addWidget(list, 1);
    root->addLayout(body, 1);
    title = UiKit::label(QStringLiteral("选择一个视频"), "heading");
    root->addWidget(title);
    auto *progress = new QHBoxLayout;
    seek = new QSlider(Qt::Horizontal);
    seek->setRange(0, 0);
    time = UiKit::label("00:00 / 00:00", "muted");
    progress->addWidget(seek, 1);
    progress->addWidget(time);
    root->addLayout(progress);
    auto *controls = new QHBoxLayout;
    auto *prev = UiKit::button(QString(), "previous");
    playButton = UiKit::button(QStringLiteral("播放"), "play");
    auto *next = UiKit::button(QString(), "next");
    controls->addWidget(prev);
    controls->addWidget(playButton);
    controls->addWidget(next);
    controls->addStretch();
    controls->addWidget(UiKit::label(QStringLiteral("音量"), "muted"));
    volume = new QSlider(Qt::Horizontal);
    volume->setMaximumWidth(160);
    volume->setRange(0, 100);
    volume->setValue(QSettings().value("audio/volume", 35).toInt());
    player.setVolume(volume->value());
    controls->addWidget(volume);
    root->addLayout(controls);
    status = UiKit::label(QStringLiteral("正在扫描视频…"), "muted");
    root->addWidget(status);
    connect(&library, &MediaLibrary::scanned, this, [this](QVector<MediaTrack> found) {
        QSignalBlocker block(list);
        tracks = found;
        list->clear();
        for (const auto &track : tracks)
            list->addItem(track.title);
        current = -1;
        scanButton->setEnabled(true);
        I18n::setText(status, tracks.isEmpty()
                                  ? QStringLiteral("暂无视频 · 将视频放入配置目录或插入 SD 卡")
                                  : QStringLiteral("共 %1 个本地视频").arg(tracks.size()));
    });
    connect(scanButton, &QPushButton::clicked, this, &OneVideo::ScanLocalVideos);
    connect(list, &QListWidget::itemClicked, this,
            [this](QListWidgetItem *item) { select(list->row(item), true); });
    connect(prev, &QPushButton::clicked, this, &OneVideo::onPrevVideo);
    connect(next, &QPushButton::clicked, this, &OneVideo::onNextVideo);
    connect(playButton, &QPushButton::clicked, this, [this]() {
        if (player.state() == QMediaPlayer::PlayingState)
            pause();
        else
            play();
    });
    connect(&player, &QMediaPlayer::stateChanged, this,
            [this](QMediaPlayer::State) { updateControls(); });
    connect(&player, &QMediaPlayer::durationChanged, this,
            [this](qint64 value) { seek->setRange(0, int(value / 1000)); });
    connect(&player, &QMediaPlayer::positionChanged, this, [this](qint64 value) {
        if (!seek->isSliderDown())
            seek->setValue(int(value / 1000));
        I18n::setText(time, durationText(value) + " / " + durationText(player.duration()));
    });
    connect(seek, &QSlider::sliderReleased, this,
            [this]() { player.setPosition(qint64(seek->value()) * 1000); });
    connect(volume, &QSlider::valueChanged, &player, &QMediaPlayer::setVolume);
    connect(&player, &QMediaPlayer::volumeChanged, this, [this](int value) {
        volume->setValue(value);
        emit volumeChanged(value);
    });
    connect(&player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus state) {
                if (state == QMediaPlayer::EndOfMedia)
                    onNextVideo();
            });
    connect(&player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this,
            [this](QMediaPlayer::Error) {
                I18n::setText(status, QStringLiteral("播放失败：%1").arg(player.errorString()));
            });
    ScanLocalVideos();
}
void OneVideo::ScanLocalVideos() {
    if (library.isScanning())
        return;
    player.stop();
    player.setMedia(QMediaContent());
    scanButton->setEnabled(false);
    library.scan("video");
}
void OneVideo::select(int index, bool start) {
    if (index < 0 || index >= tracks.size())
        return;
    current = index;
    I18n::setLiteralText(title, tracks[index].title);
    list->setCurrentRow(index);
    player.setMedia(QUrl::fromLocalFile(tracks[index].path));
    if (start)
        player.play();
}
void OneVideo::play() {
    if (tracks.isEmpty()) {
        I18n::setText(status, QStringLiteral("没有可播放的视频"));
        return;
    }
    if (current < 0)
        select(0, true);
    else
        player.play();
}
void OneVideo::onNextVideo() {
    if (!tracks.isEmpty())
        select((current + 1) % tracks.size(), true);
}
void OneVideo::onPrevVideo() {
    if (!tracks.isEmpty())
        select((qMax(0, current) - 1 + tracks.size()) % tracks.size(), true);
}
void OneVideo::updateControls() {
    const bool playing = player.state() == QMediaPlayer::PlayingState;
    I18n::setText(playButton, playing ? QStringLiteral("暂停") : QStringLiteral("播放"));
    playButton->setIcon(UiKit::icon(playing ? "pause" : "play"));
}
