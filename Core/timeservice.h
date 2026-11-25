#pragma once
#include "commandrunner.h"
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
class TimeService : public QObject {
    Q_OBJECT
public:
    explicit TimeService(QObject *parent = nullptr) : QObject(parent), runner(this) {}
    void setTime(const QDateTime &time);
    void sync();
    bool busy() const {
        return working;
    }
signals:
    void status(QString text, bool busy);

private:
    void apply(const QDateTime &time);
    void finish(const QString &text);
    CommandRunner runner;
    QNetworkAccessManager network;
    bool working = false;
};
