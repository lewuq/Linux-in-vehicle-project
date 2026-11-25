#include "Ui/uikit.h"
#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFontDatabase>
#include <QSettings>
#include <QTimer>
int main(int argc, char *argv[]) {
    if (!qEnvironmentVariableIsSet("QT_IM_MODULE"))
        qputenv("QT_IM_MODULE", "qtvirtualkeyboard");
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VehicleTerminal");
    QCoreApplication::setApplicationName("VehicleTerminal");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    if (!qEnvironmentVariableIsEmpty("VEHICLE_PREFERENCES"))
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           qEnvironmentVariable("VEHICLE_PREFERENCES"));
    app.setStyle("Fusion");
    const int fontId = QFontDatabase::addApplicationFont(":/fonts/Fonts/NotoSansCJK-Regular.ttc");
    if (fontId >= 0) {
        const auto families = QFontDatabase::applicationFontFamilies(fontId);
        for (const auto &family : families)
            if (family.contains("SC")) {
                app.setFont(QFont(family, 11));
                break;
            }
    }
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption windowed("windowed", "Run in a desktop window");
    QCommandLineOption capture("screenshots", "Capture application pages then exit", "directory");
    parser.addOption(windowed);
    parser.addOption(capture);
    parser.process(app);
    QSettings settings;
    UiKit::applyTheme(settings.value("display/night", false).toBool(),
                      settings.value("display/large", false).toBool());
    MainWindow window;
    window.setFixedSize(1024, 600);
    if (!parser.isSet(windowed))
        window.setWindowFlags(Qt::FramelessWindowHint);
    window.show();
    if (parser.isSet(capture))
        QTimer::singleShot(200, &window, [&window, &parser, &capture]() {
            window.capturePages(parser.value(capture));
        });
    return app.exec();
}
