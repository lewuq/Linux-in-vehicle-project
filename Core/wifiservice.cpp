#include "wifiservice.h"
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusVariant>
static const QString agentPath = "/org/vehicleterminal/agent";
QVariantMap WifiAgent::RequestInput(const QDBusObjectPath &, const QVariantMap &fields) {
    QVariantMap answer;
    if (fields.contains("Passphrase"))
        answer.insert("Passphrase", passphrase);
    return answer;
}
WifiService::WifiService(QObject *parent) : QObject(parent), agent(this) {
    registered = QDBusConnection::systemBus().registerObject(agentPath, &agent,
                                                             QDBusConnection::ExportAllSlots);
}
WifiService::~WifiService() {
    if (registered) {
        auto message = QDBusMessage::createMethodCall("net.connman", "/", "net.connman.Manager",
                                                      "UnregisterAgent");
        message << QVariant::fromValue(QDBusObjectPath(agentPath));
        QDBusConnection::systemBus().asyncCall(message);
        QDBusConnection::systemBus().unregisterObject(agentPath);
    }
}
void WifiService::call(const QString &path, const QString &interface, const QString &method,
                       const QVariantList &args, std::function<void(QDBusMessage)> done) {
    auto message = QDBusMessage::createMethodCall("net.connman", path, interface, method);
    message.setArguments(args);
    auto *watcher =
        new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message, 15000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [watcher, done](QDBusPendingCallWatcher *) {
                const QDBusMessage reply = watcher->reply();
                watcher->deleteLater();
                done(reply);
            });
}
void WifiService::finish(const QString &text) {
    working = false;
    message = text;
    agent.passphrase.clear();
    emit updated();
}
void WifiService::setPowered(bool powered) {
    if (working)
        return;
    working = true;
    message = powered ? QStringLiteral("正在开启 Wi-Fi…") : QStringLiteral("正在关闭 Wi-Fi…");
    emit updated();
    call("/net/connman/technology/wifi", "net.connman.Technology", "SetProperty",
         {QString("Powered"), QVariant::fromValue(QDBusVariant(powered))},
         [this, powered](QDBusMessage reply) {
             if (reply.type() == QDBusMessage::ErrorMessage) {
                 finish(QStringLiteral("Wi-Fi 操作失败，请检查设备服务"));
                 return;
             }
             if (!powered)
                 items.clear();
             finish(powered ? QStringLiteral("Wi-Fi 已开启") : QStringLiteral("Wi-Fi 已关闭"));
             if (powered)
                 scan();
         });
}
void WifiService::scan() {
    if (working)
        return;
    working = true;
    message = QStringLiteral("正在扫描附近网络…");
    emit updated();
    call("/net/connman/technology/wifi", "net.connman.Technology", "Scan", {},
         [this](QDBusMessage reply) {
             if (reply.type() == QDBusMessage::ErrorMessage) {
                 finish(QStringLiteral("扫描失败，请先开启 Wi-Fi 并检查 ConnMan"));
                 return;
             }
             refresh();
         });
}
void WifiService::refresh() {
    call("/", "net.connman.Manager", "GetServices", {}, [this](QDBusMessage reply) {
        if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) {
            finish(QStringLiteral("网络服务不可用"));
            return;
        }
        items.clear();
        const QDBusArgument argument = qvariant_cast<QDBusArgument>(reply.arguments().first());
        argument.beginArray();
        while (!argument.atEnd()) {
            QDBusObjectPath path;
            QVariantMap properties;
            argument.beginStructure();
            argument >> path >> properties;
            argument.endStructure();
            if (properties.value("Type").toString() != "wifi")
                continue;
            WifiNetwork item;
            item.path = path.path();
            item.name = properties.value("Name").toString();
            item.state = properties.value("State").toString();
            item.strength = properties.value("Strength").toInt();
            items.append(item);
        }
        argument.endArray();
        finish(items.isEmpty() ? QStringLiteral("附近暂无可用网络")
                               : QStringLiteral("找到 %1 个 Wi-Fi 网络").arg(items.size()));
    });
}
void WifiService::connectNetwork(const QString &path, const QString &password) {
    if (working || !path.startsWith("/net/connman/service/"))
        return;
    if (!registered) {
        finish(QStringLiteral("无法注册 Wi-Fi 认证代理"));
        return;
    }
    working = true;
    agent.passphrase = password;
    message = QStringLiteral("正在连接网络…");
    emit updated();
    call("/", "net.connman.Manager", "RegisterAgent",
         {QVariant::fromValue(QDBusObjectPath(agentPath))}, [this, path](QDBusMessage reply) {
             if (reply.type() == QDBusMessage::ErrorMessage &&
                 !reply.errorName().endsWith("AlreadyExists")) {
                 finish(QStringLiteral("无法启用 Wi-Fi 认证"));
                 return;
             }
             call(path, "net.connman.Service", "Connect", {}, [this](QDBusMessage result) {
                 agent.passphrase.clear();
                 if (result.type() == QDBusMessage::ErrorMessage &&
                     !result.errorName().endsWith("AlreadyConnected")) {
                     finish(QStringLiteral("连接失败，请检查密码或网络信号"));
                     return;
                 }
                 refresh();
             });
         });
}
