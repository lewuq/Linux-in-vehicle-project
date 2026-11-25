#include "searchmusic.h"
#include "Core/i18n.h"
#include "Core/network.h"
#include "Ui/uikit.h"
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QUrlQuery>
#include <QVBoxLayout>
SearchMusic::SearchMusic(QWidget *parent) : QMainWindow(parent) {
    setObjectName("page");
    UiKit::setAccent(this, "music");
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    setCentralWidget(page);
    auto *row = new QHBoxLayout;
    auto *back = UiKit::button(QStringLiteral("返回音乐"), "back");
    query = new QLineEdit;
    I18n::setProperty(query, "placeholderText", QStringLiteral("搜索歌曲或歌手"));
    searchButton = UiKit::button(QStringLiteral("搜索"), "search");
    row->addWidget(back);
    row->addWidget(query, 1);
    row->addWidget(searchButton);
    layout->addLayout(row);
    results = new QListWidget;
    layout->addWidget(results, 1);
    status =
        UiKit::label(QStringLiteral("在线结果由第三方服务提供，部分歌曲可能不可下载"), "muted");
    status->setWordWrap(true);
    layout->addWidget(status);
    auto *add = UiKit::button(QStringLiteral("添加所选歌曲到本地音乐"), "plus");
    layout->addWidget(add);
    connect(back, &QPushButton::clicked, this, &QWidget::hide);
    connect(searchButton, &QPushButton::clicked, this, &SearchMusic::search);
    connect(query, &QLineEdit::returnPressed, this, &SearchMusic::search);
    connect(add, &QPushButton::clicked, this, [this]() {
        auto *item = results->currentItem();
        if (!item)
            return;
        emit AddUrlMusic(item->text(), item->data(Qt::UserRole).toString());
        hide();
    });
    UiKit::refreshIcons(this);
}
void SearchMusic::search() {
    if (query->text().trimmed().isEmpty())
        return;
    if (active)
        active->abort();
    QUrl url("https://music.163.com/api/search/get/web");
    QUrlQuery params;
    params.addQueryItem("s", query->text().trimmed());
    params.addQueryItem("type", "1");
    params.addQueryItem("limit", "20");
    url.setQuery(params);
    QNetworkRequest request(url);
    Network::prepare(request);
    auto *reply = network.get(request);
    active = reply;
    Network::setTimeout(reply);
    searchButton->setEnabled(false);
    I18n::setText(status, QStringLiteral("正在搜索…"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (active != reply)
            return;
        active = nullptr;
        searchButton->setEnabled(true);
        if (reply->error() != QNetworkReply::NoError) {
            I18n::setText(status, QStringLiteral("搜索失败，请检查网络或稍后重试"));
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto songs = doc.object().value("result").toObject().value("songs").toArray();
        results->clear();
        for (const auto &value : songs) {
            const auto song = value.toObject();
            const auto artists = song.value("artists").toArray();
            const QString name = song.value("name").toString();
            const qint64 id = static_cast<qint64>(song.value("id").toDouble());
            if (name.isEmpty() || id <= 0)
                continue;
            const QString artist =
                artists.isEmpty() ? QString() : artists.first().toObject().value("name").toString();
            auto *item = new QListWidgetItem(name + " · " + artist, results);
            item->setData(Qt::UserRole,
                          QString("https://music.163.com/song/media/outer/url?id=%1.mp3").arg(id));
        }
        I18n::setText(status, results->count()
                                  ? QStringLiteral("找到 %1 首歌曲").arg(results->count())
                                  : QStringLiteral("暂无搜索结果或服务暂不可用"));
    });
}
