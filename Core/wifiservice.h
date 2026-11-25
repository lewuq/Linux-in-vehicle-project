#pragma once
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QObject>
#include <QVariantMap>
#include <QVector>
#include <functional>
struct WifiNetwork {
    QString name, path, state;
    int strength = 0;
};
class WifiAgent : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "net.connman.Agent")
public:
    explicit WifiAgent(QObject *parent = nullptr) : QObject(parent) {}
    QString passphrase;
public slots:
    QVariantMap RequestInput(const QDBusObjectPath &, const QVariantMap &fields);
    void Release() {
        passphrase.clear();
    }
    void Cancel() {
        passphrase.clear();
    }
    void ReportError(const QDBusObjectPath &, const QString &) {
        passphrase.clear();
    }
    void RequestBrowser(const QDBusObjectPath &, const QString &) {}
};
class WifiService : public QObject {
    Q_OBJECT
public:
    explicit WifiService(QObject *parent = nullptr);
    ~WifiService() override;
    QVector<WifiNetwork> networks() const {
        return items;
    }
    QString status() const {
        return message;
    }
    bool busy() const {
        return working;
    }
    void scan();
    void setPowered(bool powered);
    void connectNetwork(const QString &path, const QString &password);
signals:
    void updated();

private:
    void refresh();
    void finish(const QString &message);
    void call(const QString &path, const QString &interface, const QString &method,
              const QVariantList &args, std::function<void(QDBusMessage)> done);
    WifiAgent agent;
    QVector<WifiNetwork> items;
    QString message = QStringLiteral("Wi-Fi 尚未扫描");
    bool working = false, registered = false;
};
