#include "commandrunner.h"
#include <QProcess>
#include <QTimer>
#include <QVariant>
#include <memory>
void CommandRunner::run(const QString &program, const QStringList &arguments,
                        std::function<void(CommandResult)> done, int timeoutMs) {
    auto *process = new QProcess(this);
    auto *timer = new QTimer(process);
    timer->setSingleShot(true);
    const auto completed = std::make_shared<bool>(false);
    auto finish = [process, timer, completed, done](bool failedStart) {
        if (*completed)
            return;
        *completed = true;
        timer->stop();
        CommandResult result;
        bool timedOut = process->property("timedOut").toBool();
        result.ok = !timedOut && !failedStart && process->exitStatus() == QProcess::NormalExit &&
                    process->exitCode() == 0;
        result.output = QString::fromLocal8Bit(process->readAllStandardOutput()).trimmed();
        result.error = timedOut ? QStringLiteral("操作超时")
                                : QString::fromLocal8Bit(process->readAllStandardError()).trimmed();
        if (failedStart)
            result.error = process->errorString();
        process->deleteLater();
        done(result);
    };
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [finish](int, QProcess::ExitStatus) { finish(false); });
    connect(process, &QProcess::errorOccurred, this, [finish](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            finish(true);
    });
    connect(timer, &QTimer::timeout, this, [process]() {
        process->setProperty("timedOut", true);
        process->kill();
    });
    timer->start(timeoutMs);
    process->start(program, arguments);
}
