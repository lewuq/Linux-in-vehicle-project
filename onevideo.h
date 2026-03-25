#ifndef ONEVIDEO_H
#define ONEVIDEO_H
#include <QtMultimedia>
#include <QtMultimediaWidgets/QVideoWidget>

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
namespace Ui {
class OneVideo;
}
class OneVideo : public QWidget
{
    Q_OBJECT
public:
    explicit OneVideo(QWidget *parent = nullptr);
    ~OneVideo();
    void ScanLocalVideos();
private:
    QStringList candidateVideoDirs() const;
    void playVideoAt(int index);
    void updatePlayButtonIcon();

    Ui::OneVideo *ui;
    QVideoWidget *m_pPlayerWidget;// 视频显示组件
    QMediaPlayer * m_pPlayer;//媒体播放器类
    QListWidget *videoListWidget;
    QPushButton *btnPrev;
    QPushButton *btnPlayPause;
    QPushButton *btnNext;
    QPushButton *btnScan;
    QPushButton *btnDelete;
    QPushButton *btnClose;
    QPushButton *btnVolDown;
    QPushButton *btnVolUp;
    QLabel *titleLabel;
    QLabel *volumeLabel;
    QSlider *volumeSlider;
    QStringList videoFiles;
    int currentVideoIndex;

public slots:
    void PlayVideo();
    void onPlayRandomVideo();
    void onPrevVideo();
    void onNextVideo();
    void onPlayPauseVideo();
    void onVideoSelected(int row);
    void onScanVideosClicked();
    void onPlayerStateChanged(QMediaPlayer::State state);
    void onCloseClicked();
    void onDeleteVideoClicked();
    void onVolumeSliderChanged(int value);
    void onVolumeDownClicked();
    void onVolumeUpClicked();
};

#endif // ONEVIDEO_H
