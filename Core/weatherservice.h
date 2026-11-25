#pragma once
#include <QDate>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QVector>
struct DailyForecast {
    QDate date;
    QString description, icon, high, low, wind;
};
struct WeatherReport {
    QString city, updated;
    QVector<DailyForecast> days;
};
bool parseWeather(const QByteArray &json, WeatherReport *report, QString *error);
class WeatherService : public QObject {
    Q_OBJECT
public:
    explicit WeatherService(QObject *parent = nullptr);
    const WeatherReport &report() const {
        return current;
    }
    QString status() const {
        return message;
    }
    bool busy() const {
        return !active.isNull();
    }
public slots:
    void refresh();
    void setCity(const QString &city);
signals:
    void updated();

private:
    QNetworkAccessManager network;
    QPointer<QNetworkReply> active;
    WeatherReport current;
    QString message;
};
