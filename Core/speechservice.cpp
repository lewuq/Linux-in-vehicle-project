#include "speechservice.h"
#include "appconfig.h"
#include "network.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
VoiceCommand parseVoiceCommand(QString text) {
    text.remove(QRegularExpression("[\\s，。！？、,.!?]"));
    struct Rule {
        QStringList phrases;
        VoiceCommand command;
    };
    const QVector<Rule> rules = {
        {{QStringLiteral("返回首页"), QStringLiteral("回到首页"), QStringLiteral("关闭地图"),
          QStringLiteral("关闭监控"), QStringLiteral("关闭视频"), QStringLiteral("关闭设置"),
          QStringLiteral("退出音乐")},
         VoiceCommand::Home},
        {{QStringLiteral("暂停视频"), QStringLiteral("视频暂停")}, VoiceCommand::VideoPause},
        {{QStringLiteral("继续视频"), QStringLiteral("继续播放视频")}, VoiceCommand::VideoPlay},
        {{QStringLiteral("下一条视频"), QStringLiteral("下一个视频")}, VoiceCommand::VideoNext},
        {{QStringLiteral("上一条视频"), QStringLiteral("上一个视频")}, VoiceCommand::VideoPrevious},
        {{QStringLiteral("暂停音乐"), QStringLiteral("停止音乐"), QStringLiteral("暂停播放")},
         VoiceCommand::MusicPause},
        {{QStringLiteral("继续音乐"), QStringLiteral("继续播放音乐")}, VoiceCommand::MusicPlay},
        {{QStringLiteral("下一首"), QStringLiteral("下一曲"), QStringLiteral("切歌")},
         VoiceCommand::MusicNext},
        {{QStringLiteral("上一首"), QStringLiteral("上一曲")}, VoiceCommand::MusicPrevious},
        {{QStringLiteral("放大地图"), QStringLiteral("地图放大")}, VoiceCommand::MapZoomIn},
        {{QStringLiteral("缩小地图"), QStringLiteral("地图缩小")}, VoiceCommand::MapZoomOut},
        {{QStringLiteral("打开地图"), QStringLiteral("查看地图")}, VoiceCommand::Map},
        {{QStringLiteral("查看天气"), QStringLiteral("打开天气"), QStringLiteral("天气怎么样")},
         VoiceCommand::Weather},
        {{QStringLiteral("打开监控"), QStringLiteral("打开摄像头"), QStringLiteral("倒车影像")},
         VoiceCommand::Camera},
        {{QStringLiteral("播放视频"), QStringLiteral("打开视频")}, VoiceCommand::Video},
        {{QStringLiteral("打开设置"), QStringLiteral("系统设置")}, VoiceCommand::Settings},
        {{QStringLiteral("播放音乐"), QStringLiteral("打开音乐"), QStringLiteral("听歌")},
         VoiceCommand::Music}};
    for (const auto &rule : rules)
        for (const auto &phrase : rule.phrases)
            if (text.contains(phrase))
                return rule.command;
    return VoiceCommand::Unknown;
}
SpeechService::SpeechService(QObject *parent) : QObject(parent) {
    connect(&recorder, &SpeechRecognition::RecordFinished, this, &SpeechService::recognize);
    connect(&recorder, &SpeechRecognition::status, this, &SpeechService::status);
}
void SpeechService::start() {
    if (recognizing) {
        emit status(QStringLiteral("正在识别，请稍候"));
        return;
    }
    if (AppConfig::value("speech/token").isEmpty()) {
        emit status(QStringLiteral("语音服务未启用"));
        return;
    }
    recorder.startRecord();
}
void SpeechService::stop() {
    recorder.stopRecord();
}
void SpeechService::recognize() {
    if (recognizing)
        return;
    const QString token = AppConfig::value("speech/token");
    if (token.isEmpty()) {
        emit status(QStringLiteral("语音服务未启用"));
        return;
    }
    QFile file(AppConfig::recordingPath());
    if (!file.open(QIODevice::ReadOnly) || file.size() < 44 || file.size() > 10 * 1024 * 1024) {
        emit status(QStringLiteral("录音无效或时间过长"));
        return;
    }
    const QByteArray audio = file.readAll();
    QJsonObject payload{{"format", "wav"},
                        {"rate", 16000},
                        {"channel", 1},
                        {"dev_pid", 1537},
                        {"cuid", QString("vehicle-terminal")},
                        {"token", token},
                        {"speech", QString::fromLatin1(audio.toBase64())},
                        {"len", audio.size()}};
    QNetworkRequest request(QUrl("https://vop.baidu.com/server_api"));
    Network::prepare(request);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto *reply = network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    Network::setTimeout(reply, 15000);
    recognizing = true;
    emit status(QStringLiteral("正在识别…"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        recognizing = false;
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit status(QStringLiteral("语音识别失败，请检查网络"));
            return;
        }
        const auto object = QJsonDocument::fromJson(reply->readAll()).object();
        const auto results = object.value("result").toArray();
        if (object.value("err_no").toInt(-1) != 0 || results.isEmpty()) {
            emit status(QStringLiteral("没有听清，请重试"));
            return;
        }
        const QString text = results.first().toString();
        const auto value = parseVoiceCommand(text);
        emit status(value == VoiceCommand::Unknown ? QStringLiteral("暂不支持这条指令")
                                                   : QStringLiteral("已识别：%1").arg(text));
        if (value != VoiceCommand::Unknown)
            emit command(value);
    });
}
