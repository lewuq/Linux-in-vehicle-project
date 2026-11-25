#include "musicplayer.h"
#include "Core/i18n.h"
#include "Ui/uikit.h"
#include "searchmusic.h"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QSettings>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
static QString timestamp(qint64 ms) {
    return QString("%1:%2")
        .arg(ms / 60000, 2, 10, QChar('0'))
        .arg((ms / 1000) % 60, 2, 10, QChar('0'));
}
MusicPlayer::MusicPlayer(QWidget *parent) : QMainWindow(parent), library(this), downloads(this) {
    setObjectName("page");
    auto *page = new QWidget(this);
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(8, 4, 8, 8);
    root->setSpacing(16);
    setCentralWidget(page);
    auto *top = new QHBoxLayout;
    top->addWidget(UiKit::label(QStringLiteral("音乐空间"), "title"));
    top->addStretch();
    filterButton = UiKit::button(QStringLiteral("全部音乐"), "heart");
    filterButton->setCheckable(true);
    scanButton = UiKit::button(QStringLiteral("扫描媒体"), "refresh");
    auto *searchButton = UiKit::button(QStringLiteral("在线搜索"), "music");
    top->addWidget(filterButton);
    top->addWidget(scanButton);
    top->addWidget(searchButton);
    root->addLayout(top);
    auto *body = new QHBoxLayout;
    body->setSpacing(16);
    auto *art = UiKit::card();
    auto *artLayout = new QVBoxLayout(art);
    artLayout->setContentsMargins(20, 12, 20, 12);
    auto *disc = UiKit::label(QString(), "body");
    disc->setAlignment(Qt::AlignCenter);
    I18n::setPixmap(disc, UiKit::icon("music", QColor("#8a9b78"), 48).pixmap(48, 48));
    disc->setFixedHeight(50);
    artLayout->addStretch();
    artLayout->addWidget(disc);
    artLayout->addSpacing(6);
    title = UiKit::label(QStringLiteral("选择一首喜欢的歌"), "heading");
    title->setAlignment(Qt::AlignCenter);
    title->setWordWrap(true);
    subtitle = UiKit::label(QStringLiteral("本地音乐 · SD 卡 · 收藏"), "muted");
    subtitle->setAlignment(Qt::AlignCenter);
    artLayout->addWidget(title);
    artLayout->addWidget(subtitle);
    artLayout->addStretch();
    favoriteButton = UiKit::button(QStringLiteral("加入收藏"), "heart");
    favoriteButton->setCheckable(true);
    artLayout->addWidget(favoriteButton);
    list = new QListWidget;
    list->setMinimumWidth(330);
    body->addWidget(art, 4);
    body->addWidget(list, 6);
    root->addLayout(body, 1);
    auto *controls = UiKit::card();
    auto *controlLayout = new QVBoxLayout(controls);
    controlLayout->setContentsMargins(18, 8, 18, 10);
    auto *progress = new QHBoxLayout;
    seek = new QSlider(Qt::Horizontal);
    seek->setRange(0, 0);
    time = UiKit::label("00:00 / 00:00", "muted");
    progress->addWidget(seek, 1);
    progress->addWidget(time);
    controlLayout->addLayout(progress);
    auto *row = new QHBoxLayout;
    modeButton = UiKit::button(QStringLiteral("列表循环"));
    auto *prev = UiKit::button(QString(), "previous");
    auto *nextButton = UiKit::button(QString(), "next");
    playButton = UiKit::button(QStringLiteral("播放"), "play");
    playButton->setMinimumWidth(112);
    volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    volumeSlider->setMaximumWidth(150);
    row->addWidget(modeButton);
    row->addStretch();
    row->addWidget(prev);
    row->addWidget(playButton);
    row->addWidget(nextButton);
    row->addStretch();
    row->addWidget(UiKit::label(QStringLiteral("音量"), "muted"));
    row->addWidget(volumeSlider);
    controlLayout->addLayout(row);
    root->addWidget(controls);
    status = UiKit::label(QStringLiteral("正在读取媒体目录…"), "muted");
    root->addWidget(status);
    player.setPlaylist(&playlist);
    playlist.setPlaybackMode(QMediaPlaylist::Loop);
    QSettings settings;
    favorites = settings.value("music/favorites").toStringList();
    resumePosition = settings.value("music/position", 0).toLongLong();
    player.setVolume(settings.value("audio/volume", 35).toInt());
    volumeSlider->setValue(player.volume());
    connect(&library, &MediaLibrary::scanned, this, [this](QVector<MediaTrack> found) {
        const QString previousPath = playlist.currentMedia().request().url().toLocalFile();
        const bool wasPlaying = isPlaying();
        const qint64 previousPosition = player.position();
        QSignalBlocker block(list);
        tracks = found;
        playlist.clear();
        for (const auto &track : tracks)
            playlist.addMedia(QUrl::fromLocalFile(track.path));
        const QString restore =
            restoring ? QSettings().value("music/lastPath").toString() : previousPath;
        int selected = -1;
        for (int i = 0; i < tracks.size(); ++i)
            if (tracks[i].path == restore)
                selected = i;
        if (selected < 0 && !tracks.isEmpty())
            selected = 0;
        playlist.setCurrentIndex(selected);
        rebuildList();
        scanButton->setEnabled(true);
        if (!restoring && previousPosition > 0)
            player.setPosition(previousPosition);
        if (wasPlaying)
            player.play();
        I18n::setText(status,
                      tracks.isEmpty()
                          ? QStringLiteral("暂无音乐 · 将音频放入配置的音乐目录或插入 SD 卡")
                          : QStringLiteral("共 %1 首音乐 · 收藏 %2 首")
                                .arg(tracks.size())
                                .arg(favorites.size()));
        updateCurrent();
    });
    connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        restoring = false;
        playlist.setCurrentIndex(item->data(Qt::UserRole).toInt());
        player.play();
    });
    connect(scanButton, &QPushButton::clicked, this, &MusicPlayer::ScanLocalSongs);
    connect(searchButton, &QPushButton::clicked, this, [this]() {
        if (!search) {
            search = new SearchMusic(this);
            search->setWindowFlags(Qt::Widget);
            search->setGeometry(centralWidget()->rect());
            connect(search, &SearchMusic::AddUrlMusic, this,
                    [this](QString name, QString url) { downloads.download(name, QUrl(url)); });
        }
        search->setGeometry(centralWidget()->rect());
        search->show();
        search->raise();
    });
    connect(&downloads, &DownloadService::status, this,
            [this](const QString &text) { I18n::setText(status, text); });
    connect(&downloads, &DownloadService::saved, this, [this](QString) { ScanLocalSongs(); });
    connect(playButton, &QPushButton::clicked, this, &MusicPlayer::togglePlayback);
    connect(prev, &QPushButton::clicked, this, &MusicPlayer::previous);
    connect(nextButton, &QPushButton::clicked, this, &MusicPlayer::next);
    connect(filterButton, &QPushButton::toggled, this, [this](bool checked) {
        I18n::setText(filterButton,
                      checked ? QStringLiteral("我的收藏") : QStringLiteral("全部音乐"));
        rebuildList();
    });
    connect(favoriteButton, &QPushButton::clicked, this, &MusicPlayer::toggleFavorite);
    connect(modeButton, &QPushButton::clicked, this, [this]() {
        const auto current = playlist.playbackMode();
        const auto mode = current == QMediaPlaylist::Loop     ? QMediaPlaylist::Random
                          : current == QMediaPlaylist::Random ? QMediaPlaylist::CurrentItemInLoop
                                                              : QMediaPlaylist::Loop;
        playlist.setPlaybackMode(mode);
        I18n::setText(modeButton, mode == QMediaPlaylist::Loop     ? QStringLiteral("列表循环")
                                  : mode == QMediaPlaylist::Random ? QStringLiteral("随机播放")
                                                                   : QStringLiteral("单曲循环"));
    });
    connect(&playlist, &QMediaPlaylist::currentIndexChanged, this,
            [this](int) { updateCurrent(); });
    connect(&player, &QMediaPlayer::stateChanged, this,
            [this](QMediaPlayer::State) { updateCurrent(); });
    connect(&player, &QMediaPlayer::volumeChanged, this, [this](int value) {
        volumeSlider->setValue(value);
        QSettings().setValue("audio/volume", value);
        emit volumeChanged(value);
    });
    connect(volumeSlider, &QSlider::valueChanged, &player, &QMediaPlayer::setVolume);
    connect(&player, &QMediaPlayer::durationChanged, this,
            [this](qint64 duration) { seek->setRange(0, int(duration / 1000)); });
    connect(&player, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!seek->isSliderDown())
            seek->setValue(int(position / 1000));
        I18n::setText(time, timestamp(position) + " / " + timestamp(player.duration()));
    });
    connect(seek, &QSlider::sliderReleased, this,
            [this]() { player.setPosition(qint64(seek->value()) * 1000); });
    connect(&player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus state) {
                if (restoring &&
                    (state == QMediaPlayer::LoadedMedia || state == QMediaPlayer::BufferedMedia)) {
                    restoring = false;
                    player.setPosition(resumePosition);
                }
            });
    connect(&player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this,
            [this](QMediaPlayer::Error) {
                I18n::setText(status, QStringLiteral("播放失败：%1").arg(player.errorString()));
            });
    auto *saveTimer = new QTimer(this);
    saveTimer->setInterval(5000);
    connect(saveTimer, &QTimer::timeout, this, [this]() {
        if (playlist.currentIndex() >= 0) {
            QSettings s;
            s.setValue("music/lastPath", playlist.currentMedia().request().url().toLocalFile());
            s.setValue("music/position", player.position());
        }
    });
    saveTimer->start();
    ScanLocalSongs();
}
QString MusicPlayer::currentTitle() const {
    const int i = playlist.currentIndex();
    return i >= 0 && i < tracks.size() ? tracks[i].title : QStringLiteral("还没有正在播放的音乐");
}
void MusicPlayer::ScanLocalSongs() {
    if (library.isScanning()) {
        library.scan("music");
        return;
    }
    scanButton->setEnabled(false);
    I18n::setText(status, QStringLiteral("正在扫描媒体…"));
    library.scan("music");
}
void MusicPlayer::rebuildList() {
    QSignalBlocker blocker(list);
    list->clear();
    for (int i = 0; i < tracks.size(); ++i) {
        if (filterButton->isChecked() && !favorites.contains(tracks[i].path))
            continue;
        auto *item = new QListWidgetItem(
            (favorites.contains(tracks[i].path) ? QStringLiteral("♡  ") : QString()) +
                tracks[i].title,
            list);
        item->setData(Qt::UserRole, i);
        if (i == playlist.currentIndex())
            list->setCurrentItem(item);
    }
}
void MusicPlayer::updateCurrent() {
    if (tracks.isEmpty())
        I18n::setText(title, currentTitle());
    else
        I18n::setLiteralText(title, currentTitle());
    I18n::setText(playButton, isPlaying() ? QStringLiteral("暂停") : QStringLiteral("播放"));
    playButton->setIcon(UiKit::icon(isPlaying() ? "pause" : "play"));
    const QString path = playlist.currentMedia().request().url().toLocalFile();
    favoriteButton->setChecked(favorites.contains(path));
    I18n::setText(favoriteButton,
                  favorites.contains(path) ? QStringLiteral("已收藏") : QStringLiteral("加入收藏"));
    favoriteButton->setEnabled(!path.isEmpty());
    emit nowPlayingChanged(currentTitle(), isPlaying());
}
void MusicPlayer::toggleFavorite() {
    const QString path = playlist.currentMedia().request().url().toLocalFile();
    if (path.isEmpty())
        return;
    if (favorites.contains(path))
        favorites.removeAll(path);
    else
        favorites.append(path);
    QSettings().setValue("music/favorites", favorites);
    rebuildList();
    updateCurrent();
}
void MusicPlayer::play() {
    if (!playlist.isEmpty()) {
        if (playlist.currentIndex() < 0)
            playlist.setCurrentIndex(0);
        player.play();
    }
}
void MusicPlayer::togglePlayback() {
    if (isPlaying())
        player.pause();
    else
        play();
}
void MusicPlayer::next() {
    playlist.next();
    play();
}
void MusicPlayer::previous() {
    playlist.previous();
    play();
}
void MusicPlayer::playRandomSong() {
    if (!playlist.isEmpty()) {
        playlist.setCurrentIndex(QRandomGenerator::global()->bounded(playlist.mediaCount()));
        player.play();
    }
}
