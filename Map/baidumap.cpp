#include "baidumap.h"
#include "Core/appconfig.h"
#include "Core/gnssservice.h"
#include "Core/network.h"
#include "Ui/uikit.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSettings>
#include <QUrlQuery>
#include <QVBoxLayout>
BaiduMap::BaiduMap(GnssService *service, QWidget *parent) : QMainWindow(parent), gnss(service) {
    setObjectName("page");
    auto *page = new QWidget;
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(8, 4, 8, 8);
    root->setSpacing(14);
    setCentralWidget(page);
    auto *row = new QHBoxLayout;
    row->addWidget(UiKit::label(QStringLiteral("地图与位置"), "title"));
    row->addStretch();
    search = new QLineEdit;
    search->setPlaceholderText(QStringLiteral("搜索城市或地点"));
    search->setMaximumWidth(220);
    auto *go = UiKit::button(QStringLiteral("查看"), "pin");
    auto *locate = UiKit::button(QStringLiteral("我的位置"), "map");
    row->addWidget(search);
    row->addWidget(go);
    row->addWidget(locate);
    root->addLayout(row);
    auto *card = UiKit::card();
    auto *layout = new QVBoxLayout(card);
    image = UiKit::label(QStringLiteral("搜索地点，或开启 GNSS 查看当前位置"));
    image->setAlignment(Qt::AlignCenter);
    image->setWordWrap(true);
    image->setMinimumHeight(230);
    layout->addWidget(image, 1);
    root->addWidget(card, 1);
    auto *bottom = new QHBoxLayout;
    auto *column = new QVBoxLayout;
    status = UiKit::label(QString(), "body");
    coordinates = UiKit::label(QString(), "muted");
    column->addWidget(status);
    column->addWidget(coordinates);
    bottom->addLayout(column, 1);
    auto *bookmark = UiKit::button(QStringLiteral("保存地点"), "heart");
    auto *home = UiKit::button(QStringLiteral("收藏地点"), "pin");
    auto *minus = UiKit::button(QString(), "minus");
    auto *plus = UiKit::button(QString(), "plus");
    bottom->addWidget(bookmark);
    bottom->addWidget(home);
    bottom->addWidget(minus);
    bottom->addWidget(plus);
    root->addLayout(bottom);
    connect(go, &QPushButton::clicked, this, [this]() {
        center = search->text().trimmed();
        requestMap(false);
    });
    connect(search, &QLineEdit::returnPressed, go, &QPushButton::click);
    connect(locate, &QPushButton::clicked, this, [this]() {
        if (!gnss->isEnabled())
            gnss->setEnabled(true);
        requestMap(true);
    });
    connect(plus, &QPushButton::clicked, this, &BaiduMap::zoomIn);
    connect(minus, &QPushButton::clicked, this, &BaiduMap::zoomOut);
    connect(bookmark, &QPushButton::clicked, this, [this]() {
        if (center.isEmpty())
            return;
        QSettings s;
        s.setValue("map/bookmark", center);
        s.setValue("map/bookmarkFix", usingFix);
        status->setText(QStringLiteral("地点已保存"));
    });
    connect(home, &QPushButton::clicked, this, [this]() {
        QSettings s;
        center = s.value("map/bookmark").toString();
        if (center.isEmpty()) {
            status->setText(QStringLiteral("尚未保存地点"));
            return;
        }
        usingFix = s.value("map/bookmarkFix", false).toBool();
        requestMap(false);
    });
    connect(gnss, &GnssService::updated, this, &BaiduMap::updateFix);
    updateFix();
}
void BaiduMap::updateFix() {
    const auto fix = gnss->fix();
    coordinates->setText(fix.valid ? QStringLiteral("WGS84  %1, %2 · %3")
                                         .arg(fix.latitude, 0, 'f', 6)
                                         .arg(fix.longitude, 0, 'f', 6)
                                         .arg(gnss->status())
                                   : gnss->status());
}
void BaiduMap::zoomIn() {
    zoom = qMin(18, zoom + 1);
    requestMap(false);
}
void BaiduMap::zoomOut() {
    zoom = qMax(3, zoom - 1);
    requestMap(false);
}
void BaiduMap::requestMap(bool useFix) {
    if (useFix) {
        const auto fix = gnss->fix();
        if (!fix.valid) {
            status->setText(QStringLiteral("尚未获得有效定位"));
            return;
        }
        center = QString("%1,%2").arg(fix.longitude, 0, 'f', 6).arg(fix.latitude, 0, 'f', 6);
        usingFix = true;
    } else if (!search->text().trimmed().isEmpty() && center == search->text().trimmed())
        usingFix = false;
    if (center.isEmpty())
        return;
    const QString key = AppConfig::value("map/api_key");
    if (key.isEmpty()) {
        status->setText(QStringLiteral("地图服务未启用"));
        return;
    }
    if (active) {
        active->disconnect(this);
        active->abort();
        active->deleteLater();
    }
    QUrl url("https://api.map.baidu.com/staticimage/v2");
    QUrlQuery params;
    params.addQueryItem("ak", key);
    params.addQueryItem("width", "800");
    params.addQueryItem("height", "350");
    params.addQueryItem("center", center);
    params.addQueryItem("markers", center);
    params.addQueryItem("zoom", QString::number(zoom));
    if (usingFix)
        params.addQueryItem("coordtype", "wgs84ll");
    url.setQuery(params);
    QNetworkRequest request(url);
    Network::prepare(request);
    auto *reply = network.get(request);
    active = reply;
    Network::setTimeout(reply);
    status->setText(QStringLiteral("正在加载地图…"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (active != reply)
            return;
        active = nullptr;
        QPixmap pix;
        if (reply->error() != QNetworkReply::NoError || !pix.loadFromData(reply->readAll())) {
            status->setText(QStringLiteral("地图加载失败，请检查网络或服务配置"));
            return;
        }
        image->setPixmap(pix.scaled(image->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        status->setText(QStringLiteral("%1 · 缩放 %2").arg(center).arg(zoom));
    });
}
