#include "Music/musicplayer.h"
#include "ui_musicplayer.h"

#include <QMouseEvent>
#include <QLocale>
#include <QFileInfo>
#include <algorithm>
#include <QCollator>
#include <QSet>
#include <QTextCodec>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QRandomGenerator>

static QString resolveSongDir(const QString &rawPath)
{
    if (QDir::isAbsolutePath(rawPath)) {
        return rawPath;
    }
    return QCoreApplication::applicationDirPath() + "/" + rawPath;
}

static QString sanitizeSongName(const QString &name)
{
    QString safe = name;
    safe.replace("/", "_");
    safe.replace("\\", "_");
    safe.replace(":", "_");
    safe.replace("*", "_");
    safe.replace("?", "_");
    safe.replace("\"", "_");
    safe.replace("<", "_");
    safe.replace(">", "_");
    safe.replace("|", "_");
    return safe.trimmed();
}

static int countCjk(const QString &text)
{
    int count = 0;
    for (int i = 0; i < text.size(); ++i) {
        const ushort u = text.at(i).unicode();
        if ((u >= 0x4E00 && u <= 0x9FFF) || (u >= 0x3400 && u <= 0x4DBF)) {
            ++count;
        }
    }
    return count;
}

static int garblePenalty(const QString &text)
{
    int penalty = 0;
    for (int i = 0; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (ch == QChar::ReplacementCharacter || ch == QLatin1Char('?')) {
            penalty += 3;
        }
    }
    return penalty;
}

static int mojibakePenalty(const QString &text)
{
    int penalty = 0;
    for (int i = 0; i < text.size(); ++i) {
        const ushort u = text.at(i).unicode();
        // 常见 UTF-8/Latin1 串码字符区间，中文歌名中大量出现通常是乱码。
        if (u >= 0x00C0 && u <= 0x00FF) {
            penalty += 2;
        }
    }
    if (text.contains(QString::fromUtf8("锟")) || text.contains(QString::fromUtf8("烫"))) {
        penalty += 8;
    }
    return penalty;
}

static int readableScore(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        return 1 << 30;
    }

    int alphaNum = 0;
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i).isLetterOrNumber()) {
            ++alphaNum;
        }
    }

    const int penalty = garblePenalty(text) * 100 + mojibakePenalty(text) * 10;
    const int bonus = countCjk(text) * 3 + alphaNum;
    return penalty - bonus;
}

static QString tryLatin1RoundTripUtf8(const QString &text)
{
    const QByteArray latin1 = text.toLatin1();
    if (latin1.isEmpty()) {
        return text;
    }
    return QString::fromUtf8(latin1);
}

static QString chooseReadableName(const QStringList &candidates, const QString &fallback)
{
    QString best = fallback;
    int bestScore = readableScore(best);
    for (int i = 0; i < candidates.size(); ++i) {
        const QString candidate = candidates.at(i).trimmed();
        if (candidate.isEmpty()) {
            continue;
        }
        const int score = readableScore(candidate);
        if (score < bestScore) {
            best = candidate;
            bestScore = score;
        }
    }
    return best;
}

static QString decodeFat32NameFallback(const QString &name)
{
    const QByteArray raw = QFile::encodeName(name);
    QString utf8Name = QString::fromUtf8(raw);
    QString localeName = QString::fromLocal8Bit(raw);
    QString gbName;
    QTextCodec *gbCodec = QTextCodec::codecForName("GB18030");
    if (gbCodec) {
        gbName = gbCodec->toUnicode(raw);
    }

    // 覆盖常见路径：UTF-8/本地编码/GB18030/Latin1 误解码回转。
    QStringList candidates;
    candidates << name
               << utf8Name
               << localeName
               << gbName
               << tryLatin1RoundTripUtf8(name)
               << tryLatin1RoundTripUtf8(utf8Name)
               << tryLatin1RoundTripUtf8(localeName)
               << tryLatin1RoundTripUtf8(gbName);

    return chooseReadableName(candidates, name);
}

static bool isLikelyGarbledName(const QString &name)
{
    if (name.trimmed().isEmpty()) {
        return true;
    }

    int badCount = 0;
    for (int i = 0; i < name.size(); ++i) {
        const QChar ch = name.at(i);
        if (ch == QChar::ReplacementCharacter || ch == QLatin1Char('?')) {
            ++badCount;
        }
    }
    if (name.contains(QString::fromUtf8("锟")) || name.contains(QString::fromUtf8("烫"))) {
        badCount += 2;
    }
    return badCount > 0;
}

static quint32 readBigEndianU32(const uchar *p)
{
    return (quint32(p[0]) << 24)
         | (quint32(p[1]) << 16)
         | (quint32(p[2]) << 8)
         | quint32(p[3]);
}

static quint32 readSynchsafeU32(const uchar *p)
{
    return (quint32(p[0] & 0x7F) << 21)
         | (quint32(p[1] & 0x7F) << 14)
         | (quint32(p[2] & 0x7F) << 7)
         | quint32(p[3] & 0x7F);
}

static QString cleanupTagText(QString s)
{
    s.replace(QChar('\0'), QChar(' '));
    s = s.simplified();
    return s;
}

static QString decodeId3TextPayload(const QByteArray &payload)
{
    if (payload.isEmpty()) {
        return QString();
    }

    const uchar encoding = uchar(payload.at(0));
    const QByteArray textData = payload.mid(1);
    QString result;

    switch (encoding) {
    case 0: {
        QTextCodec *gbCodec = QTextCodec::codecForName("GB18030");
        const QString gb = gbCodec ? gbCodec->toUnicode(textData) : QString();
        const QString latin1 = QString::fromLatin1(textData);
        result = chooseReadableName(QStringList() << gb << latin1, latin1);
        break;
    }
    case 1: {
        if (textData.size() >= 2) {
            const uchar b0 = uchar(textData.at(0));
            const uchar b1 = uchar(textData.at(1));
            if (b0 == 0xFF && b1 == 0xFE) {
                result = QString::fromUtf16(reinterpret_cast<const ushort *>(textData.constData() + 2),
                                            (textData.size() - 2) / 2);
            } else if (b0 == 0xFE && b1 == 0xFF) {
                QByteArray le;
                le.resize(textData.size() - 2);
                for (int i = 2; i + 1 < textData.size(); i += 2) {
                    le[i - 2] = textData.at(i + 1);
                    le[i - 1] = textData.at(i);
                }
                result = QString::fromUtf16(reinterpret_cast<const ushort *>(le.constData()), le.size() / 2);
            } else {
                result = QString::fromUtf16(reinterpret_cast<const ushort *>(textData.constData()), textData.size() / 2);
            }
        }
        break;
    }
    case 2: {
        QByteArray le;
        le.resize(textData.size());
        for (int i = 0; i + 1 < textData.size(); i += 2) {
            le[i] = textData.at(i + 1);
            le[i + 1] = textData.at(i);
        }
        result = QString::fromUtf16(reinterpret_cast<const ushort *>(le.constData()), le.size() / 2);
        break;
    }
    case 3:
        result = QString::fromUtf8(textData);
        break;
    default:
        break;
    }

    return cleanupTagText(result);
}

static QString readId3TitleFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    const QByteArray header = file.read(10);
    if (header.size() >= 10 && header.mid(0, 3) == "ID3") {
        const int majorVersion = uchar(header.at(3));
        const quint32 tagSize = readSynchsafeU32(reinterpret_cast<const uchar *>(header.constData() + 6));
        const qint64 tagEnd = qMin<qint64>(file.size(), 10 + qint64(tagSize));

        while (file.pos() + 10 <= tagEnd) {
            const QByteArray frameHeader = file.read(10);
            if (frameHeader.size() < 10) {
                break;
            }

            if (frameHeader.at(0) == '\0' && frameHeader.at(1) == '\0'
                && frameHeader.at(2) == '\0' && frameHeader.at(3) == '\0') {
                break;
            }

            const QByteArray frameId = frameHeader.left(4);
            const uchar *sizeBytes = reinterpret_cast<const uchar *>(frameHeader.constData() + 4);
            quint32 frameSize = (majorVersion >= 4) ? readSynchsafeU32(sizeBytes) : readBigEndianU32(sizeBytes);

            if (frameSize == 0 || file.pos() + frameSize > tagEnd) {
                break;
            }

            const QByteArray payload = file.read(frameSize);
            if (frameId == "TIT2") {
                const QString title = decodeId3TextPayload(payload);
                if (!title.isEmpty() && !isLikelyGarbledName(title)) {
                    file.close();
                    return title;
                }
            }
        }
    }

    // ID3v1 兜底（文件尾 128 字节）
    if (file.size() >= 128 && file.seek(file.size() - 128)) {
        const QByteArray tag = file.read(128);
        if (tag.size() == 128 && tag.mid(0, 3) == "TAG") {
            const QByteArray titleBytes = tag.mid(3, 30).trimmed();
            QTextCodec *gbCodec = QTextCodec::codecForName("GB18030");
            const QString gb = gbCodec ? gbCodec->toUnicode(titleBytes) : QString();
            const QString latin1 = QString::fromLatin1(titleBytes);
            const QString title = cleanupTagText(chooseReadableName(QStringList() << gb << latin1, latin1));
            if (!title.isEmpty() && !isLikelyGarbledName(title)) {
                file.close();
                return title;
            }
        }
    }

    file.close();
    return QString();
}

static bool looksLikeCopyrightBlocked(const QNetworkReply *reply, const QByteArray &data)
{
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString contentType = QString::fromLatin1(reply->header(QNetworkRequest::ContentTypeHeader).toByteArray()).toLower();
    const QString finalUrl = reply->url().toString().toLower();

    if (status == 403 || status == 404 || status == 451) {
        return true;
    }

    if (finalUrl.contains("404") || finalUrl.contains("forbidden") || finalUrl.contains("copyright") || finalUrl.contains("vip")) {
        return true;
    }

    if (contentType.startsWith("text/html") || contentType.startsWith("application/json")) {
        return true;
    }

    // 过小的“音频”通常是错误页或跳转页。
    if (data.size() < 2048) {
        return true;
    }

    return false;
}

MusicPlayer::MusicPlayer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MusicPlayer),
    volumeHud(nullptr),
    volumeValueLabel(nullptr),
    volumeProgressBar(nullptr),
    volumeHudHideTimer(nullptr)
{
    ui->setupUi(this);
    this->setMinimumSize(1024,600);

    // 主控按钮大一号，次控按钮小一号。
    ui->pBtn_Pre->setFixedSize(72, 72);
    ui->pBtn_Next->setFixedSize(72, 72);
    ui->pBtn_OpenSearchWin->setFixedSize(72, 72);
    ui->pBtn_Pause->setFixedSize(84, 84);
    ui->pBtn_Pre->setIconSize(QSize(72, 72));
    ui->pBtn_Next->setIconSize(QSize(72, 72));
    ui->pBtn_OpenSearchWin->setIconSize(QSize(72, 72));
    ui->pBtn_Pause->setIconSize(QSize(84, 84));

    ui->pBtn_SetLove->setFixedSize(56, 56);
    ui->pBtn_loop->setFixedSize(56, 56);
    ui->pBtn_Loud->setFixedSize(56, 56);
    ui->pBtn_Low->setFixedSize(56, 56);
    ui->pBtn_SetLove->setIconSize(QSize(56, 56));
    ui->pBtn_loop->setIconSize(QSize(56, 56));
    ui->pBtn_Loud->setIconSize(QSize(56, 56));
    ui->pBtn_Low->setIconSize(QSize(56, 56));
    ui->pBtn_loop->setCheckable(true);
    
    // 内核不负责 Qt 文本绘制，字体由 main.cpp 的内置字体加载统一处理。
    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));
    
    searchMusicWin = new SearchMusic(this);
    mediaPlayerInit();
    initVolumeHud();
    loadSongNameCache();
    ScanLocalSongs();
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    Second_request.setSslConfiguration(config);
    // 清理 ui 文件中的内联样式，确保外部 QSS 能完整接管界面风格。
    const QList<QWidget *> widgets = this->findChildren<QWidget *>();
    for (QWidget *w : widgets) {
        w->setStyleSheet(QString());
    }

    ui->label->setText(QString::fromUtf8("网易云音乐"));

    QPushButton *deleteBtn = new QPushButton(QString::fromUtf8("删除歌曲"), this);
    deleteBtn->setObjectName("pBtn_Delete");
    deleteBtn->setMinimumSize(110, 46);
    deleteBtn->setStyleSheet("QPushButton#pBtn_Delete{background:#fdeceb;color:#d33a31;border:1px solid #f1c5c2;border-radius:10px;font-size:16px;font-weight:700;padding:4px 10px;}"
                             "QPushButton#pBtn_Delete:pressed{background:#f8ddd9;}");
    ui->horizontalLayout_4->addWidget(deleteBtn);
    connect(deleteBtn, SIGNAL(clicked()), this, SLOT(on_pBtn_Delete_clicked()));

    QFile style_file(":/music/Music/style.qss");
    if(style_file.exists())
    {
        style_file.open(QFile::ReadOnly);
        QString styleStr = QLatin1String(style_file.readAll());
        this->setStyleSheet(styleStr);
    }

    connect(searchMusicWin,&SearchMusic::AddUrlMusic,this,&MusicPlayer::AddMusicFromUrl);
    connect(musicPlayer,SIGNAL(durationChanged(qint64)),this,SLOT(on_musicPlayer_DurationChanged(qint64)));
    connect(musicPlayer,SIGNAL(positionChanged(qint64)),this,SLOT(on_musicPlayer_CurPostionChanged(qint64)));
    connect(&First_netManager,SIGNAL(finished(QNetworkReply* )),this,SLOT(on_GetSongTrueUrl(QNetworkReply* )));
    connect(&Second_netManager,SIGNAL(finished(QNetworkReply* )),this,SLOT(on_DownSong(QNetworkReply* )));

}

MusicPlayer::~MusicPlayer()
{
    delete ui;
}

/*  扫描本地音乐 - 支持多路径搜索 (本地 + SD卡)  */
void MusicPlayer::ScanLocalSongs()
{
    // 定义多个可能的音乐目录路径（优先级从高到低）
    QStringList musicPaths;
    musicPaths << "/run/media/mmcblk0p1"      // SD卡挂载点 (IMX6ULL)
              << "/run/media/mmcblk0p1/Music" // SD卡Music子目录
              << "/mnt/mmc0"                  // 备用SD卡挂载点
              << "/mnt/mmc0/Music"            // 备用SD卡Music子目录
              << "/mnt/mmcblk0/Music"         // 备用SD卡路径
              << "/mnt/sd/Music"              // 备用SD卡路径
              << "/media/sdcard/Music"        // 备用SD卡路径
              << resolveSongDir(LocalSongsPath); // 本地目录
    
    QStringList filter;
    filter << "*.mp3" << "*.MP3" << "*.wav" << "*.WAV";
    
    QVector<MediaObjectInfo> scannedSongs;
    QSet<QString> seenPaths;
    bool cacheUpdated = false;

    // 遍历所有音乐路径
    for(const QString &musicPath : musicPaths)
    {
        QDir dir(musicPath);
        if(dir.exists())
        {
            qDebug() << "找到音乐目录: " << musicPath;
            QFileInfoList files = dir.entryInfoList(filter, QDir::Files);
            
            if(files.count() > 0)
            {
                qDebug() << "在 " << musicPath << " 找到 " << files.count() << " 首音乐";
            }
            
            for(int i = 0; i < files.count(); i++)
            {
                MediaObjectInfo info;
                info.filePath = files.at(i).filePath();
                const QString canonicalPath = QFileInfo(info.filePath).canonicalFilePath();
                const QString dedupKey = canonicalPath.isEmpty() ? info.filePath : canonicalPath;
                if (seenPaths.contains(dedupKey)) {
                    continue;
                }
                seenPaths.insert(dedupKey);

                const QString baseName = files.at(i).completeBaseName();
                const QString decodedName = decodeFat32NameFallback(baseName);
                info.fileName = restoreSongDisplayName(info.filePath, decodedName);

                if (isLikelyGarbledName(info.fileName)) {
                    const QString id3Title = readId3TitleFromFile(info.filePath);
                    if (!id3Title.isEmpty()) {
                        info.fileName = id3Title;
                    }
                }

                const QString songKey = normalizedSongKey(info.filePath);
                if (!songKey.isEmpty()
                    && !isLikelyGarbledName(info.fileName)
                    && songNameCache.value(songKey) != info.fileName) {
                    songNameCache.insert(songKey, info.fileName);
                    cacheUpdated = true;
                }
                scannedSongs.append(info);
            }
        }
    }

    if (cacheUpdated) {
        saveSongNameCache();
    }

    QCollator collator(QLocale(QLocale::Chinese, QLocale::China));
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);
    std::sort(scannedSongs.begin(), scannedSongs.end(),
              [&collator](const MediaObjectInfo &a, const MediaObjectInfo &b) {
        return collator.compare(a.fileName, b.fileName) < 0;
    });

    SongInfoVector.clear();
    ui->listWidget->clear();
    musicPlayList->clear();
    for (int i = 0; i < scannedSongs.size(); ++i) {
        const MediaObjectInfo &info = scannedSongs.at(i);
        if (musicPlayList->addMedia(QUrl::fromLocalFile(info.filePath))) {
            SongInfoVector.append(info);
            ui->listWidget->addItem(info.fileName);
            qDebug() << "加载音乐: " << info.filePath;
        } else {
            qDebug() << musicPlayList->errorString();
            qDebug() << "Error number:" << musicPlayList->error();
        }
    }
    
    qDebug() << "总共加载了" << SongInfoVector.count() << "首音乐";
}

/* 初始化音乐播放器  */
void MusicPlayer::mediaPlayerInit()
{
    musicPlayer = new QMediaPlayer(this);
    musicPlayList = new QMediaPlaylist(this);
    musicPlayList->clear();
    musicPlayer->setPlaylist(musicPlayList);
    musicPlayList->setPlaybackMode(QMediaPlaylist::Loop);
    musicPlayer->setVolume(20);
}

QString MusicPlayer::songNameCachePath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("song_name_cache.json");
}

QString MusicPlayer::normalizedSongKey(const QString &filePath) const
{
    const QFileInfo info(filePath);
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}

QString MusicPlayer::restoreSongDisplayName(const QString &filePath, const QString &fallback) const
{
    const QString key = normalizedSongKey(filePath);
    if (songNameCache.contains(key)) {
        const QString cached = songNameCache.value(key).trimmed();
        if (!cached.isEmpty()) {
            return cached;
        }
    }
    return fallback;
}

void MusicPlayer::rememberSongDisplayName(const QString &filePath, const QString &displayName)
{
    const QString cleanName = displayName.trimmed();
    if (cleanName.isEmpty() || isLikelyGarbledName(cleanName)) {
        return;
    }

    const QString key = normalizedSongKey(filePath);
    if (key.isEmpty()) {
        return;
    }
    songNameCache.insert(key, cleanName);
}

void MusicPlayer::loadSongNameCache()
{
    songNameCache.clear();
    QFile file(songNameCachePath());
    if (!file.exists()) {
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[SongNameCache] 打开失败:" << file.fileName();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    for (QJsonObject::const_iterator it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const QString value = it.value().toString().trimmed();
        if (!value.isEmpty()) {
            songNameCache.insert(it.key(), value);
        }
    }
}

void MusicPlayer::saveSongNameCache() const
{
    QJsonObject obj;
    for (QHash<QString, QString>::const_iterator it = songNameCache.constBegin(); it != songNameCache.constEnd(); ++it) {
        if (!it.value().trimmed().isEmpty()) {
            obj.insert(it.key(), it.value());
        }
    }

    QSaveFile file(songNameCachePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "[SongNameCache] 写入失败:" << file.fileName();
        return;
    }
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    file.commit();
}

void MusicPlayer::initVolumeHud()
{
    volumeHud = new QWidget(this);
    volumeHud->setObjectName("volumeHud");
    volumeHud->setAttribute(Qt::WA_StyledBackground, true);
    volumeHud->setFixedSize(82, 220);

    QVBoxLayout *layout = new QVBoxLayout(volumeHud);
    layout->setContentsMargins(12, 14, 12, 14);
    layout->setSpacing(10);

    volumeProgressBar = new QProgressBar(volumeHud);
    volumeProgressBar->setObjectName("volumeHudBar");
    volumeProgressBar->setRange(0, 100);
    volumeProgressBar->setValue(musicPlayer ? musicPlayer->volume() : 50);
    volumeProgressBar->setOrientation(Qt::Vertical);
    volumeProgressBar->setTextVisible(false);
    volumeProgressBar->setMinimumHeight(140);

    volumeValueLabel = new QLabel(volumeHud);
    volumeValueLabel->setObjectName("volumeHudValue");
    volumeValueLabel->setAlignment(Qt::AlignCenter);
    volumeValueLabel->setText(QString::number(musicPlayer ? musicPlayer->volume() : 50));

    layout->addWidget(volumeProgressBar, 1);
    layout->addWidget(volumeValueLabel, 0, Qt::AlignCenter);

    volumeHud->setStyleSheet(
        "QWidget#volumeHud{"
        "background:rgba(12,18,28,220);"
        "border:1px solid rgba(255,255,255,45);"
        "border-radius:16px;}"
        "QLabel#volumeHudValue{"
        "color:#ffffff;font-size:22px;font-weight:700;}"
        "QProgressBar#volumeHudBar{"
        "background:rgba(255,255,255,30);"
        "border:none;border-radius:10px;}"
        "QProgressBar#volumeHudBar::chunk{"
        "background:#32c05f;border-radius:10px;}"
    );

    volumeHudHideTimer = new QTimer(this);
    volumeHudHideTimer->setSingleShot(true);
    connect(volumeHudHideTimer, &QTimer::timeout, volumeHud, &QWidget::hide);
    volumeHud->hide();
}

void MusicPlayer::updateVolumeHud(int volume)
{
    if (!volumeHud || !volumeProgressBar || !volumeValueLabel) {
        return;
    }

    volumeProgressBar->setValue(volume);
    volumeValueLabel->setText(QString::number(volume));

    const int x = qMax(10, width() - volumeHud->width() - 16);
    const int y = qMax(10, (height() - volumeHud->height()) / 2);
    volumeHud->move(x, y);
    volumeHud->raise();
    volumeHud->show();

    if (volumeHudHideTimer) {
        volumeHudHideTimer->start(3000);
    }
}

void MusicPlayer::changeVolumeByStep(int delta)
{
    if (!musicPlayer) {
        return;
    }
    const int targetVolume = qBound(0, musicPlayer->volume() + delta, 100);
    musicPlayer->setVolume(targetVolume);
    updateVolumeHud(targetVolume);
    qDebug() << "Curr Volume" << targetVolume;
}

/*  打开在线搜索音乐界面  */
void MusicPlayer::on_pBtn_OpenSearchWin_clicked()
{
    searchMusicWin->show();
}


/*  根据音乐url地址下载音乐 添加到本地  */
void MusicPlayer::AddMusicFromUrl(QString name, QString UrlPath)
{
    const QUrl url = QUrl::fromUserInput(UrlPath.trimmed());
    if (!url.isValid() || UrlPath.trimmed().isEmpty()) {
        ui->label->setText(QString::fromUtf8("歌曲添加失败：下载地址无效"));
        return;
    }

    CurrentSaveSongFileName = sanitizeSongName(name);
    if (CurrentSaveSongFileName.isEmpty()) {
        CurrentSaveSongFileName = QString::fromUtf8("未命名歌曲");
    }

    qDebug() << "Download request:" << url;
    First_request.setUrl(url);
    First_netManager.get(First_request);
    ui->label->setText(QString::fromUtf8("正在解析下载地址..."));
}


/*  用户点击其他音乐进行播放  */
void MusicPlayer::on_listWidget_currentRowChanged(int currentRow)
{
    musicPlayer->stop();
    musicPlayList->setCurrentIndex(currentRow);
    musicPlayer->play();
}

/* 点击切换上一首按钮 */
void MusicPlayer::on_pBtn_Pre_clicked()
{
    if (musicPlayList->isEmpty()) {
        return;
    }

    musicPlayer->stop();
    int currentIndex = musicPlayList->currentIndex();
    if (currentIndex < 0) {
        currentIndex = ui->listWidget->currentRow();
    }
    if (currentIndex < 0) {
        currentIndex = 0;
    }
    const int previousIndex = (currentIndex - 1 + musicPlayList->mediaCount()) % musicPlayList->mediaCount();
    musicPlayList->setCurrentIndex(previousIndex);
    ui->listWidget->setCurrentRow(previousIndex);
    musicPlayer->play();
}

/* 点击切换下一首按钮 */
void MusicPlayer::on_pBtn_Next_clicked()
{
    if (musicPlayList->isEmpty()) {
        return;
    }

    musicPlayer->stop();
    int currentIndex = musicPlayList->currentIndex();
    if (currentIndex < 0) {
        currentIndex = ui->listWidget->currentRow();
    }
    if (currentIndex < 0) {
        currentIndex = -1;
    }
    const int nextIndex = (currentIndex + 1) % musicPlayList->mediaCount();
    musicPlayList->setCurrentIndex(nextIndex);
    ui->listWidget->setCurrentRow(nextIndex);
    musicPlayer->play();
}


/* 暂停音乐 */
void MusicPlayer::on_pBtn_Pause_clicked()
{
    if(ui->pBtn_Pause->isChecked())
    {
        musicPlayer->stop();
    }
    else
        musicPlayer->play();
}


/*  增加音量  */
void MusicPlayer::on_pBtn_Loud_clicked()
{
    changeVolumeByStep(+10);
}


/* 降低音量 */
void MusicPlayer::on_pBtn_Low_clicked()
{
    changeVolumeByStep(-10);
}




/* 上下滑动音乐列表时 */
void MusicPlayer::on_horizontalSlider_sliderReleased()
{
    musicPlayer->setPosition(ui->horizontalSlider->value()*1000);
}


/* 切换音乐时更新音乐总时长  */
void MusicPlayer::on_musicPlayer_DurationChanged(qint64 duration)
{
    ui->horizontalSlider->setMaximum(duration/1000);
    int min = duration/1000/60;
    int sec = (duration/1000)%60;
    QString minStr = QString::number(min);
    QString secStr = QString::number(sec);
    if(min<10)minStr="0"+minStr;
    if(sec<10)secStr="0"+secStr;
    QString durationShow = QString("%1:%2").arg(minStr,secStr);
    ui->label_TotalTime->setText(durationShow);
}

/*  */
void MusicPlayer::on_musicPlayer_CurPostionChanged(qint64 value )
{
    ui->horizontalSlider->setValue(value/1000);
    int min = value/1000/60;
    int sec = (value/1000)%60;
    QString minStr = QString::number(min);
    QString secStr = QString::number(sec);
    if(min<10)minStr="0"+minStr;
    if(sec<10)secStr="0"+secStr;
    QString curTime = QString("%1:%2").arg(minStr,secStr);
    ui->label_CurTime->setText(curTime);
}


/* 点击循环播放按钮 */
void MusicPlayer::on_pBtn_loop_clicked()
{
    if(ui->pBtn_loop->isChecked())
    {
        musicPlayList->setPlaybackMode(QMediaPlaylist::Random);
        ui->label->setText(QString::fromUtf8("随机播放"));
    }
    else
    {
        musicPlayList->setPlaybackMode(QMediaPlaylist::Loop);
        ui->label->setText(QString::fromUtf8("列表循环"));
    }
}


/* 根据音乐搜索网络请求，获取音乐的真实下载地址 */
void MusicPlayer::on_GetSongTrueUrl(QNetworkReply *reply)
{
    qDebug()<<"on_GetSongTrueUrl";
    if (reply->error() != QNetworkReply::NoError) {
        ui->label->setText(QString::fromUtf8("歌曲添加失败：网络错误"));
        reply->deleteLater();
        return;
    }

    const QVariant redirectAttr = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectAttr.isValid()) {
        QUrl redirectUrl = redirectAttr.toUrl();
        if (redirectUrl.isRelative()) {
            redirectUrl = reply->url().resolved(redirectUrl);
        }
        if (!redirectUrl.isValid()) {
            ui->label->setText(QString::fromUtf8("歌曲添加失败：重定向地址无效"));
            reply->deleteLater();
            return;
        }
        Second_request.setUrl(redirectUrl);
        Second_netManager.get(Second_request);
        ui->label->setText(QString::fromUtf8("正在下载歌曲..."));
        reply->deleteLater();
        return;
    }

    qDebug()<<reply->rawHeaderList();
    qDebug()<<reply->rawHeaderPairs();
    qDebug()<<reply->readAll();
    int total = reply->rawHeaderPairs().length();
    qDebug()<<total;
    for(int i=0;i<total;i++)
        {
            QString first = QString(reply->rawHeaderPairs().at(i).first);
            if(first.compare(QString("Location"))==0)
            {
                QString urlDownload = QString(reply->rawHeaderPairs().at(i).second);
                if(urlDownload.endsWith("404"))
                {
                    QMessageBox::warning(this,"warning","该歌曲无法下载，请换一首");
                    return;
                }
                qDebug()<<urlDownload;
                Second_request.setUrl(QUrl(urlDownload));
                Second_netManager.get(Second_request);
                ui->label->setText(QString::fromUtf8("正在下载歌曲..."));
                reply->deleteLater();
                return;
            }
        }

    ui->label->setText(QString::fromUtf8("歌曲添加失败：未获取到下载地址"));
    reply->deleteLater();
}


/* 下载音乐到本地文件 */
void MusicPlayer::on_DownSong(QNetworkReply *reply)
{
    // 1) 网络错误
    if (reply->error() != QNetworkReply::NoError) {
        ui->label->setText(QString::fromUtf8("歌曲添加失败：下载错误"));
        qDebug() << "[AddSong][网络错误]" << reply->errorString();
        reply->deleteLater();
        return;
    }

    const QString baseDir = resolveSongDir(LocalSongsPath);
    QDir dir(baseDir);
    if (!dir.exists() && !dir.mkpath(".")) {
        ui->label->setText(QString::fromUtf8("歌曲添加失败：目录不可写"));
        reply->deleteLater();
        return;
    }

    const QByteArray data = reply->readAll();

    // 2) 权限/版权限制
    if (looksLikeCopyrightBlocked(reply, data)) {
        ui->label->setText(QString::fromUtf8("歌曲添加失败：权限/版权限制"));
        qDebug() << "[AddSong][权限/版权限制]"
                 << "status=" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                 << "contentType=" << reply->header(QNetworkRequest::ContentTypeHeader)
                 << "url=" << reply->url();
        reply->deleteLater();
        return;
    }

    if (data.isEmpty()) {
        ui->label->setText(QString::fromUtf8("歌曲添加失败：下载内容为空"));
        qDebug() << "[AddSong][网络错误] 下载内容为空";
        reply->deleteLater();
        return;
    }

    QFile file;
    // 构建保存路径，正确处理中文文件名
    QString FilePath = baseDir + "/" + CurrentSaveSongFileName + ".mp3";
    file.setFileName(FilePath);
    // 3) 文件写入失败
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ui->label->setText(QString::fromUtf8("歌曲添加失败：文件写入失败"));
        qDebug() << "[AddSong][文件写入失败]" << FilePath;
        reply->deleteLater();
        return;
    }
    const qint64 written = file.write(data);
    file.close();
    if (written != data.size()) {
        ui->label->setText(QString::fromUtf8("歌曲添加失败：文件写入失败"));
        qDebug() << "[AddSong][文件写入失败] 写入字节不完整"
                 << "expected=" << data.size() << "written=" << written;
        reply->deleteLater();
        return;
    }
    qDebug()<<"Save Song <"<<CurrentSaveSongFileName<<"> "<<"Finished";
    MediaObjectInfo info;
    info.fileName = CurrentSaveSongFileName;
    info.filePath = FilePath;
    rememberSongDisplayName(info.filePath, info.fileName);
    saveSongNameCache();
    SongInfoVector.append(info);

    QCollator collator(QLocale(QLocale::Chinese, QLocale::China));
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);
    std::sort(SongInfoVector.begin(), SongInfoVector.end(),
              [&collator](const MediaObjectInfo &a, const MediaObjectInfo &b) {
        return collator.compare(a.fileName, b.fileName) < 0;
    });

    musicPlayList->clear();
    ui->listWidget->clear();
    int newIndex = -1;
    for (int i = 0; i < SongInfoVector.size(); ++i) {
        const MediaObjectInfo &song = SongInfoVector.at(i);
        if (!musicPlayList->addMedia(QUrl::fromLocalFile(song.filePath))) {
            qDebug()<<musicPlayList->errorString()<<endl;
            qDebug()<<"Error number:"<<musicPlayList->error()<<endl;
            ui->label->setText(QString::fromUtf8("歌曲添加失败：播放器无法加载"));
            reply->deleteLater();
            return;
        }
        ui->listWidget->addItem(song.fileName);
        if (song.filePath == FilePath) {
            newIndex = i;
        }
    }

    if (newIndex >= 0) {
        musicPlayList->setCurrentIndex(newIndex);
        ui->listWidget->setCurrentRow(newIndex);
    }
    ui->label->setText(QString::fromUtf8("歌曲添加成功"));

    reply->deleteLater();
}


/* 退出按钮  */
void MusicPlayer::on_pushButton_clicked()
{
    this->hide();
}

void MusicPlayer::on_pBtn_Delete_clicked()
{
    const int index = ui->listWidget->currentRow();
    if (index < 0 || index >= SongInfoVector.size()) {
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择要删除的歌曲"));
        return;
    }

    const QString filePath = SongInfoVector.at(index).filePath;
    const QString songKey = normalizedSongKey(filePath);
    const int ret = QMessageBox::question(this, QString::fromUtf8("确认删除"),
                                          QString::fromUtf8("确定删除该歌曲吗？\n%1").arg(SongInfoVector.at(index).fileName),
                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    musicPlayer->stop();

    if (QFile::exists(filePath) && !QFile::remove(filePath)) {
        ui->label->setText(QString::fromUtf8("删除失败：文件不可删除"));
        QMessageBox::warning(this, QString::fromUtf8("删除失败"), QString::fromUtf8("文件删除失败，请检查权限"));
        return;
    }

    musicPlayList->removeMedia(index);
    SongInfoVector.remove(index);
    delete ui->listWidget->takeItem(index);
    if (!songKey.isEmpty() && songNameCache.contains(songKey)) {
        songNameCache.remove(songKey);
        saveSongNameCache();
    }

    if (!musicPlayList->isEmpty()) {
        const int nextIndex = qMin(index, musicPlayList->mediaCount() - 1);
        ui->listWidget->setCurrentRow(nextIndex);
        musicPlayList->setCurrentIndex(nextIndex);
        musicPlayer->play();
    } else {
        ui->label_CurTime->setText("00:00");
        ui->label_TotalTime->setText("00:00");
    }

    ui->label->setText(QString::fromUtf8("歌曲删除成功"));
}


/* 处理语音控制传来的指令 */
void MusicPlayer::on_handleCommand(int command)
{
    switch (command) {
    case MUSIC_COMMAND_SHOW:
        this->show();
        break;
    case MUSIC_COMMAND_CLOSE:
        this->hide();
        break;
    case MUSIC_COMMAND_PAUSE:
        musicPlayer->pause();
        break;
    case MUSIC_COMMAND_PLAY:
        if(musicPlayList->isEmpty())return;
        if (musicPlayList->currentIndex() < 0) {
            musicPlayList->setCurrentIndex(0);
            ui->listWidget->setCurrentRow(0);
        }
        musicPlayer->play();
        break;
    case MUSIC_COMMAND_PRE:
        on_pBtn_Pre_clicked();
        break;
    case MUSIC_COMMAND_NEXT:
        on_pBtn_Next_clicked();
        break;
    default:
        break;
    }
}

void MusicPlayer::playRandomSong()
{
    if (musicPlayList->isEmpty()) {
        ScanLocalSongs();
    }

    if (musicPlayList->isEmpty()) {
        ui->label->setText(QString::fromUtf8("未找到可播放音乐"));
        return;
    }

    const int randomIndex = QRandomGenerator::global()->bounded(musicPlayList->mediaCount());
    musicPlayList->setCurrentIndex(randomIndex);
    ui->listWidget->setCurrentRow(randomIndex);
    musicPlayer->play();
    ui->label->setText(QString::fromUtf8("随机播放"));
}
