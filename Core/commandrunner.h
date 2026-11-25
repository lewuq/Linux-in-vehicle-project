#pragma once
#include <QObject>
#include <QStringList>
#include <functional>
struct CommandResult {
    bool ok = false;
    QString output;
    QString error;
};
class CommandRunner : public QObject {
    Q_OBJECT
public:
    explicit CommandRunner(QObject *parent = nullptr) : QObject(parent) {}
    void run(const QString &program, const QStringList &arguments,
             std::function<void(CommandResult)> done, int timeoutMs = 10000);
};
