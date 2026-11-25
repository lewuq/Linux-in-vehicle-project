#pragma once
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QPointer>
class GnssService;
class QLabel;
class QLineEdit;
class QNetworkReply;
class BaiduMap : public QMainWindow {
    Q_OBJECT
public:
    explicit BaiduMap(GnssService *service, QWidget *parent = nullptr);
    void zoomIn();
    void zoomOut();

private:
    void requestMap(bool useFix = false);
    void updateFix();
    GnssService *gnss;
    QNetworkAccessManager network;
    QPointer<QNetworkReply> active;
    QLabel *image, *status, *coordinates;
    QLineEdit *search;
    QString center;
    bool usingFix = false;
    int zoom = 14;
};
