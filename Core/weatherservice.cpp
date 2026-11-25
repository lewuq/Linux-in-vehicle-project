#include "weatherservice.h"
#include "appconfig.h"
#include "network.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTimer>
#include <QUrlQuery>
bool parseWeather(const QByteArray &json, WeatherReport *report, QString *error) {
    if (!report)
        return false;
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(json, &parseError);
    auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return fail(QStringLiteral("天气数据格式异常"));
    WeatherReport parsed;
    const auto object = doc.object();
    parsed.city = object.value("city").toString();
    parsed.updated = object.value("update_time").toString();
    const auto days = object.value("data").toArray();
    for (const auto &value : days) {
        const auto item = value.toObject();
        DailyForecast day;
        day.date = QDate::fromString(item.value("date").toString(), Qt::ISODate);
        day.description = item.value("wea").toString();
        day.icon = item.value("wea_img").toString();
        day.high = item.value("tem_day").toVariant().toString();
        day.low = item.value("tem_night").toVariant().toString();
        day.wind = item.value("win").toString() + " " + item.value("win_speed").toString();
        if (!day.date.isValid() || day.description.isEmpty() || day.high.isEmpty() ||
            day.low.isEmpty())
            continue;
        parsed.days.append(day);
        if (parsed.days.size() >= 7)
            break;
    }
    if (parsed.city.isEmpty() || parsed.days.isEmpty())
        return fail(QStringLiteral("天气服务未返回有效预报"));
    *report = parsed;
    if (error)
        error->clear();
    return true;
}
WeatherService::WeatherService(QObject *parent) : QObject(parent) {
    auto *timer = new QTimer(this);
    timer->setInterval(30 * 60 * 1000);
    connect(timer, &QTimer::timeout, this, &WeatherService::refresh);
    timer->start();
}
void WeatherService::setCity(const QString &city) {
    const QString clean = city.trimmed();
    if (clean.isEmpty())
        return;
    QSettings().setValue("weather/city", clean);
    if (active) {
        active->disconnect(this);
        active->abort();
        active->deleteLater();
        active = nullptr;
    }
    current = WeatherReport{};
    refresh();
}
void WeatherService::refresh() {
    if (active)
        return;
    const QString id = AppConfig::value("weather/app_id"),
                  secret = AppConfig::value("weather/app_secret");
    if (id.isEmpty() || secret.isEmpty()) {
        message = QStringLiteral("天气服务未启用");
        emit updated();
        return;
    }
    QUrl url("https://v1.yiketianqi.com/free/week");
    QUrlQuery params;
    params.addQueryItem("appid", id);
    params.addQueryItem("appsecret", secret);
    params.addQueryItem("unescape", "1");
    params.addQueryItem(
        "city", QSettings()
                    .value("weather/city", AppConfig::value("weather/city", QStringLiteral("惠州")))
                    .toString());
    url.setQuery(params);
    QNetworkRequest request(url);
    Network::prepare(request);
    auto *reply = network.get(request);
    active = reply;
    Network::setTimeout(reply);
    message = QStringLiteral("正在更新天气…");
    emit updated();
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        active = nullptr;
        QString error;
        WeatherReport next;
        if (reply->error() != QNetworkReply::NoError)
            message = QStringLiteral("天气更新失败，请稍后重试");
        else if (parseWeather(reply->readAll(), &next, &error)) {
            current = next;
            message = QStringLiteral("更新于 %1").arg(current.updated);
        } else
            message = error;
        reply->deleteLater();
        emit updated();
    });
}
