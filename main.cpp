#include "mainwindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QFont>
#include <QScreen>
#include "clock.h"

// ===== 字体加载函数（军规四：独立字体文件） =====
void loadEmbeddedFonts() {
    const int fontId = QFontDatabase::addApplicationFont(":/fonts/NotoSansCJK-Regular.ttc");
    if (fontId == -1) {
        qWarning() << "[启动] 内置中文字体加载失败: :/fonts/NotoSansCJK-Regular.ttc";
        return;
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) {
        qWarning() << "[启动] 内置字体已加载但未返回可用字族";
        return;
    }

    QString selectedFamily;
    for (int i = 0; i < families.size(); ++i) {
        if (families.at(i).contains("SC", Qt::CaseInsensitive)) {
            selectedFamily = families.at(i);
            break;
        }
    }
    if (selectedFamily.isEmpty()) {
        selectedFamily = families.first();
    }

    QFont appFont = QApplication::font();
    appFont.setFamily(selectedFamily);
    if (appFont.pointSize() < 10) {
        appFont.setPointSize(10);
    }
    QApplication::setFont(appFont);
    qDebug() << "[启动] 已启用内置中文字体:" << selectedFamily;
}

// ===== 全屏窗口设置函数（军规二：禁用窗口装饰） =====
void configureEmbeddedDisplay(QMainWindow *mainWin) {
    // 设置固定尺寸（i.MX6ULL标准分辨率）
    mainWin->setFixedSize(1024, 600);
    
    // 移除窗口装饰（最大化、最小化、关闭按钮）
    // 在嵌入式LinuxFB环境中，窗口管理器本身不存在，但这确保跨平台兼容性
    mainWin->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    
    // 移到屏幕中央（某些嵌入式屏幕可能默认不居中）
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - 1024) / 2;
        int y = (screenGeometry.height() - 600) / 2;
        mainWin->move(x, y);
    }
    
    qDebug() << "[启动] 嵌入式窗口配置完成: 1024x600 / 固定尺寸 / 无窗口装饰";
}

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    // 桌面模式输入面板：可悬浮/可拖动，避免占满全屏。
    qputenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE", QByteArray("0"));
    // 约 1/3 屏幕观感，避免遮挡主要内容。
    qputenv("QT_VIRTUALKEYBOARD_SCALE", QByteArray("0.35"));
    // 默认中文输入环境（可在键盘上切换英文）。
    qputenv("QT_VIRTUALKEYBOARD_LOCALE", QByteArray("zh_CN"));
    QApplication a(argc, argv);
    
    // ===== 第一步：加载字体（军规四） =====
    loadEmbeddedFonts();
    
    // ===== 第二步：创建主窗口 =====
    MainWindow w;
    
    // ===== 第三步：配置嵌入式显示（军规二） =====
    configureEmbeddedDisplay(&w);
    
    // ===== 第四步：显示主窗口 =====
    w.show();
    
    return a.exec();
}
