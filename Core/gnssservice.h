#pragma once
#include "Map/gps.h"
#include <QObject>
#include <QTimer>
class GnssService : public QObject {
    Q_OBJECT
public:
    explicit GnssService(QObject *parent = nullptr);
    ~GnssService() override;
    GpsFix fix() const {
        return current;
    }
    QString status() const {
        return message;
    }
    bool isEnabled() const {
        return enabled;
    }
public slots:
    void setEnabled(bool active);
signals:
    void updated();

private:
    void refresh();
    QTimer timer;
    GpsFix current{};
    QString message = QStringLiteral("定位未开启");
    bool enabled = false;
};
