#pragma once
#include <QThread>
class Dht11 : public QThread {
    Q_OBJECT
public:
    explicit Dht11(QObject *parent = nullptr) : QThread(parent) {}
    ~Dht11() override {
        requestInterruption();
        wait();
    }
signals:
    void updateDht11Data(QString humidity, QString temperature);
    void unavailable(QString reason);

protected:
    void run() override;
};
