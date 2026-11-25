#include "Core/commandrunner.h"
#include "Core/downloadservice.h"
#include "Core/i18n.h"
#include "Core/medialibrary.h"
#include "Core/speechservice.h"
#include "Core/weatherservice.h"
#include "Map/gps.h"
#include "Ui/uikit.h"
#include "mainwindow.h"
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QtTest>

class FakeReply : public QNetworkReply {
public:
    explicit FakeReply(const QNetworkRequest &request, QObject *parent) : QNetworkReply(parent) {
        setRequest(request);
        setUrl(request.url());
        open(QIODevice::ReadOnly);
    }
    void complete(const QByteArray &bytes) {
        data = bytes;
        emit readyRead();
        setFinished(true);
        emit finished();
    }
    void abort() override {
        setError(OperationCanceledError, "cancelled");
        setFinished(true);
        emit finished();
    }
    qint64 bytesAvailable() const override {
        return data.size() + QNetworkReply::bytesAvailable();
    }

protected:
    qint64 readData(char *destination, qint64 max) override {
        const qint64 count = qMin(max, qint64(data.size()));
        if (!count)
            return -1;
        memcpy(destination, data.constData(), size_t(count));
        data.remove(0, int(count));
        return count;
    }

private:
    QByteArray data;
};
class FakeNetwork : public QNetworkAccessManager {
public:
    QVector<FakeReply *> replies;

protected:
    QNetworkReply *createRequest(Operation, const QNetworkRequest &request, QIODevice *) override {
        auto *reply = new FakeReply(request, this);
        replies.append(reply);
        return reply;
    }
};
class VehicleTests : public QObject {
    Q_OBJECT
    QTemporaryDir temporary;
    static QByteArray sentence(const QByteArray &body) {
        unsigned char sum = 0;
        for (const auto c : body)
            sum ^= static_cast<unsigned char>(c);
        return "$" + body + "*" + QByteArray::number(sum, 16).rightJustified(2, '0').toUpper();
    }
private slots:
    void initTestCase() {
        QVERIFY(temporary.isValid());
        qputenv("VEHICLE_PATHS_DATA", temporary.path().toUtf8());
        qputenv("VEHICLE_HARDWARE_ENABLED", "false");
        qputenv("VEHICLE_WEATHER_APP_ID", "");
        qputenv("VEHICLE_WEATHER_APP_SECRET", "");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
        QCoreApplication::setOrganizationName("VehicleTests");
        QCoreApplication::setApplicationName("VehicleTests");
        const int id = QFontDatabase::addApplicationFont(":/fonts/Fonts/NotoSansCJK-Regular.ttc");
        QVERIFY(id >= 0);
        for (const auto &family : QFontDatabase::applicationFontFamilies(id))
            if (family.contains("SC")) {
                qApp->setFont(QFont(family, 11));
                break;
            }
        UiKit::applyTheme();
        QVERIFY(qApp->styleSheet().contains("#f5f4f0"));
    }
    void gpsCorrectCoordinates() {
        GpsFix fix{};
        const auto valid =
            sentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
        QVERIFY(gps_parse_sentence(valid.constData(), &fix));
        QVERIFY(fix.valid);
        QVERIFY(qAbs(fix.latitude - 48.1173) < 0.000001);
        QVERIFY(qAbs(fix.longitude - 11.5166666667) < 0.000001);
        QCOMPARE(fix.satellites, 8);
    }
    void gpsSouthernWestern() {
        GpsFix fix{};
        const auto valid = sentence("GNRMC,123519,A,2230.000,S,11430.000,W,0,0,010126,,");
        QVERIFY(gps_parse_sentence(valid.constData(), &fix));
        QVERIFY(qAbs(fix.latitude + 22.5) < 0.000001);
        QVERIFY(qAbs(fix.longitude + 114.5) < 0.000001);
    }
    void gpsMalformedFrames() {
        GpsFix fix{};
        QVERIFY(!gps_parse_sentence("$GPGGA*00", &fix));
        QVERIFY(!gps_parse_sentence(sentence("GPGGA,1,9161.0,N,01131.0,E,1,08").constData(), &fix));
        QVERIFY(!gps_parse_sentence(sentence("GPGGA,1,4807.0,X,01131.0,E,1,08").constData(), &fix));
        QVERIFY(!gps_parse_sentence(QByteArray(200, 'x').constData(), &fix));
        QVERIFY(!gps_parse_sentence(sentence("GPGGA,missing").constData(), &fix));
    }
    void gpsNoFix() {
        GpsFix fix{};
        QVERIFY(gps_parse_sentence(sentence("GPGGA,1,,,,,0,00").constData(), &fix));
        QVERIFY(!fix.valid);
    }
    void weatherShortReport() {
        WeatherReport report;
        QString error;
        const QByteArray json =
            R"({"city":"Huizhou","data":[{"date":"2026-11-25","wea":"Sunny","wea_img":"qing","tem_day":24,"tem_night":"18"}]})";
        QVERIFY(parseWeather(json, &report, &error));
        QCOMPARE(report.days.size(), 1);
        QCOMPARE(report.days[0].high, QString("24"));
    }
    void weatherRejectsMalformed() {
        WeatherReport report;
        QString error;
        for (const auto &json : QList<QByteArray>{
                 "<html>error</html>", "[]", "{}", R"({"city":"A","data":[{}]})",
                 R"({"city":"A","data":[{"date":"bad","wea":"A","tem_day":"1","tem_night":"0"}]})"})
            QVERIFY(!parseWeather(json, &report, &error));
    }
    void voiceSpecificCommands() {
        QVERIFY(parseVoiceCommand(QStringLiteral("请暂停视频。")) == VoiceCommand::VideoPause);
        QVERIFY(parseVoiceCommand(QStringLiteral("继续播放视频")) == VoiceCommand::VideoPlay);
        QVERIFY(parseVoiceCommand(QStringLiteral("播放视频")) == VoiceCommand::Video);
        QVERIFY(parseVoiceCommand(QStringLiteral("下一首")) == VoiceCommand::MusicNext);
        QVERIFY(parseVoiceCommand(QStringLiteral("无匹配内容")) == VoiceCommand::Unknown);
    }
    void downloadsKeepRequestIdentity() {
        FakeNetwork network;
        DownloadService service(nullptr, &network);
        QSignalSpy saved(&service, &DownloadService::saved);
        service.download("Track A", QUrl("https://audio.test/a"));
        service.download("Track B", QUrl("https://audio.test/b"));
        QCOMPARE(network.replies.size(), 2);
        const QByteArray b = "ID3-content-B", a = "ID3-content-A";
        network.replies[1]->complete(b);
        network.replies[0]->complete(a);
        QCOMPARE(saved.count(), 2);
        for (const auto &result : saved) {
            const QString path = result[0].toString();
            QFile file(path);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), path.contains("Track A") ? a : b);
        }
    }
    void downloadsRejectErrorBodies() {
        FakeNetwork network;
        DownloadService service(nullptr, &network);
        QSignalSpy saved(&service, &DownloadService::saved);
        service.download("Error", QUrl("https://audio.test/error"));
        network.replies.first()->complete("<html>not audio</html>");
        QCOMPARE(saved.count(), 0);
        QSignalSpy status(&service, &DownloadService::status);
        service.download("unsafe", QUrl("http://audio.test/a"));
        QCOMPARE(status.count(), 1);
        QCOMPARE(network.replies.size(), 1);
    }
    void commandFailureCompletesOnce() {
        CommandRunner runner;
        int calls = 0;
        runner.run("vehicle-nonexistent-command-xyz", {}, [&calls](CommandResult result) {
            QVERIFY(!result.ok);
            ++calls;
        });
        QTRY_COMPARE(calls, 1);
        QTest::qWait(20);
        QCOMPARE(calls, 1);
    }
    void mediaScanDeduplicatesDirectories() {
        QTemporaryDir media;
        QFile file(media.path() + "/sample.mp3");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("ID3test");
        file.close();
        qputenv("VEHICLE_PATHS_MUSIC", (media.path() + ";" + media.path()).toUtf8());
        MediaLibrary library;
        QVector<MediaTrack> result;
        bool done = false;
        connect(&library, &MediaLibrary::scanned, this, [&](QVector<MediaTrack> tracks) {
            result = tracks;
            done = true;
        });
        library.scan("music");
        QTRY_VERIFY(done);
        int matches = 0;
        for (const auto &track : result)
            if (track.title == "sample")
                ++matches;
        QCOMPARE(matches, 1);
        qunsetenv("VEHICLE_PATHS_MUSIC");
    }
    void mediaScanQueuesRefreshRequests() {
        MediaLibrary library;
        int completions = 0;
        connect(&library, &MediaLibrary::scanned, this,
                [&completions](QVector<MediaTrack>) { ++completions; });
        library.scan("music");
        library.scan("music");
        QTRY_COMPARE(completions, 2);
    }

    void languageDefaultsToChinese() {
        QSettings().remove("display/language");
        I18n::instance().initialize();
        QCOMPARE(I18n::instance().language(), QString("zh_CN"));
        QCOMPARE(I18n::instance().text(QStringLiteral("设置中心")), QStringLiteral("设置中心"));
    }
    void languageBindsExistingAndNewWidgets() {
        auto *label = UiKit::label(QStringLiteral("正在下载 %1").arg(QStringLiteral("首页%1")));
        QLineEdit input;
        I18n::setProperty(&input, "placeholderText", QStringLiteral("搜索城市或地点"));
        I18n::instance().setLanguage("en_US");
        QCOMPARE(label->text(), QStringLiteral("Downloading 首页%1"));
        QCOMPARE(input.placeholderText(), QString("Search a city or place"));
        auto *button = UiKit::button(QStringLiteral("扫描网络"));
        QCOMPARE(button->text(), QString("Scan networks"));
        QCOMPARE(I18n::instance().text(QStringLiteral("内存    当前平台不提供内存数据")),
                 QString("Memory    Memory data unavailable on this platform"));
        I18n::instance().setLanguage("zh_CN");
        QCOMPARE(label->text(), QStringLiteral("正在下载 首页%1"));
        QCOMPARE(button->text(), QStringLiteral("扫描网络"));
        delete label;
        delete button;
    }
    void languagePreservesMediaAndImages() {
        QLabel title;
        I18n::setText(&title, QStringLiteral("选择一个视频"));
        I18n::setLiteralText(&title, QStringLiteral("首页"));
        QLabel image;
        I18n::setText(&image, QStringLiteral("等待摄像头"));
        QPixmap frame(24, 24);
        frame.fill(Qt::green);
        I18n::setPixmap(&image, frame);
        I18n::instance().setLanguage("en_US");
        QCOMPARE(title.text(), QStringLiteral("首页"));
        QCOMPARE(image.pixmap(Qt::ReturnByValue).size(), QSize(24, 24));
        I18n::instance().setLanguage("zh_CN");
        QCOMPARE(image.pixmap(Qt::ReturnByValue).size(), QSize(24, 24));
    }
    void languagePersistsAndRejectsInvalidPreference() {
        I18n::instance().setLanguage("en_US");
        QCOMPARE(QSettings().value("display/language").toString(), QString("en_US"));
        I18n::instance().initialize();
        QCOMPARE(I18n::instance().language(), QString("en_US"));
        QSettings().setValue("display/language", "unsupported");
        I18n::instance().initialize();
        QCOMPARE(I18n::instance().language(), QString("zh_CN"));
        QSettings().remove("display/language");
    }
    void settingsSwitchLanguageWithoutRecreatingPages() {
        MainWindow window;
        window.setFixedSize(1024, 600);
        window.show();
        window.openPage(6);
        auto *stack = window.findChild<QStackedWidget *>();
        QWidget *settings = stack->currentWidget();
        auto *english = window.findChild<QPushButton *>("languageEnglish");
        auto *chinese = window.findChild<QPushButton *>("languageChinese");
        QVERIFY(english && chinese);
        auto *tabs = settings->findChild<QTabWidget *>();
        auto *password = settings->findChild<QLineEdit *>("wifiPassword");
        QVERIFY(password);
        password->setText("unmodified-input");
        QTest::mouseClick(english, Qt::LeftButton);
        QCOMPARE(stack->currentWidget(), settings);
        QCOMPARE(tabs->tabText(0), QString("Network"));
        QCOMPARE(password->placeholderText(), QString("Password for selected network"));
        QCOMPARE(password->text(), QString("unmodified-input"));
        QVERIFY(english->isChecked());
        QCOMPARE(window.findChild<QPushButton *>("nav_home")->accessibleName(), QString("Home"));
        const int pageCount = stack->count();
        window.openPage(4);
        bool translatedWeather = false;
        for (auto *label : stack->currentWidget()->findChildren<QLabel *>())
            translatedWeather |= label->text() == "Weather ahead";
        QVERIFY(translatedWeather);
        window.openPage(6);
        QTest::mouseClick(chinese, Qt::LeftButton);
        QCOMPARE(stack->currentWidget(), settings);
        QCOMPARE(stack->count(), pageCount + 1);
        QCOMPARE(tabs->tabText(0), QStringLiteral("网络连接"));
        QCOMPARE(password->text(), QString("unmodified-input"));
        QVERIFY(chinese->isChecked());
        QCOMPARE(window.findChild<QPushButton *>("nav_home")->accessibleName(),
                 QStringLiteral("首页"));
    }

    void shellLazyLoadsAndKeepsNavigation() {
        MainWindow window;
        window.setFixedSize(1024, 600);
        window.show();
        auto *stack = window.findChild<QStackedWidget *>();
        QVERIFY(stack);
        QCOMPARE(stack->count(), 1);
        for (int i = 0; i < 7; ++i) {
            window.openPage(i);
            QTest::qWait(50);
            QCOMPARE(window.size(), QSize(1024, 600));
            QVERIFY(stack->currentWidget()->isVisible());
        }
        QCOMPARE(stack->count(), 7);
        for (auto *button : window.findChildren<QPushButton *>()) {
            if (button->isVisible() && button->objectName().startsWith("nav_")) {
                const QPoint point = button->mapTo(&window, QPoint(0, 0));
                QVERIFY(window.rect().contains(QRect(point, button->size())));
                QLabel *glyph = nullptr;
                QLabel *caption = nullptr;
                for (auto *label : button->findChildren<QLabel *>()) {
                    if (!label->property("uiIconName").toString().isEmpty())
                        glyph = label;
                    if (!label->text().isEmpty())
                        caption = label;
                }
                QVERIFY(glyph && caption);
                QVERIFY(button->rect().contains(glyph->geometry()));
                QVERIFY(button->rect().contains(caption->geometry()));
                QVERIFY(!glyph->geometry().intersects(caption->geometry()));
            }
        }
        window.openPage(0);
        QCOMPARE(stack->currentWidget()->objectName(), QString("homePage"));
    }
};
QTEST_MAIN(VehicleTests)
#include "tst_vehicle.moc"
