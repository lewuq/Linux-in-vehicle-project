# 车载终端嵌入式开发四项工程准则

**目标平台：** i.MX6ULL  
**显示屏幕：** 1024x600 / LinuxFB（无窗口管理器）  
**物理内存：** 512MB（实际可用约 300-350MB）  
**开发语言：** Qt/C++

---

## 准则一：避免使用本地绝对路径 🚫

### 现象与问题
```
❌ PC 开发环境 (Ubuntu)：一切正常
✓ 图片显示正常
✓ 字体加载正常
✓ 样式表应用正常

❌ 硬件部署 (i.MX6ULL)：全部崩溃
✗ 图标全变空白
✗ 字体加载失败，显示豆腐块
✗ 样式表找不到文件
```

**根本原因：** 硬件和PC的文件系统路径完全不同
```
PC 上：      /home/zeller/Qt_test/VehicleTerminal/img/weather.jpg
硬件上可能： /app/bin/resources/ 或 /etc/app/assets/
```

### 规范：完全依赖 Qt 资源系统 (.qrc)

#### ✅ 正确做法

**1. 所有资源都纳入 .qrc 文件**

```xml
<!-- img.qrc -->
<RCC>
    <qresource prefix="/weather">
        <file>Weather/images/qing.png</file>
        <file>Weather/images/yu.png</file>
        <file>Weather/weather_dark_style.qss</file>
    </qresource>
    <qresource prefix="/fonts">
        <file>Fonts/SourceHanSansCN-Bold.ttf</file>
        <file>Fonts/SourceHanSansCN-Regular.ttf</file>
    </qresource>
</RCC>
```

**2. 代码中使用 :/ 前缀的资源路径**

```cpp
// ❌ 错误：硬编码绝对路径
QString imagePath = "/home/zeller/Qt_test/VehicleTerminal/Weather/images/qing.png";
pixmap.load(imagePath);  // 在硬件上会崩溃！

// ✅ 正确：使用资源路径
QString imagePath = ":/weather/Weather/images/qing.png";
pixmap.load(imagePath);  // PC 和硬件上都能工作

// ✅ 正确：样式表加载
QFile styleFile(":/weather/Weather/weather_dark_style.qss");
if (styleFile.open(QFile::ReadOnly)) {
    this->setStyleSheet(QLatin1String(styleFile.readAll()));
    styleFile.close();
}

// ✅ 正确：字体加载
int fontId = QFontDatabase::addApplicationFont(":/fonts/SourceHanSansCN-Bold.ttf");
```

#### 检查清单

- [x] 所有 PNG/JPG 图片都在 ..qrc 中声明
- [x] 所有 QSS 样式表都在 .qrc 中声明
- [x] 所有 TTF/OTF 字体都在 .qrc 中声明
- [x] 代码中不存在 `/home/`, `/usr/`, `/opt/` 路径

---

## 准则二：全屏与多窗口管理策略 📱

### 现象与问题

```
❌ PC 开发环境：多窗口独立弹出，完全正常
✓ 主窗口 + 天气窗口 + 地图窗口 可随意拖动
✓ 对话框弹出流畅，关键信息清晰

❌ 硬件环境 (LinuxFB)：界面卡死或显示错乱
✗ 没有窗口管理器，不支持拖拽和独立窗口
✗ QDialog 弹窗可能导致主窗口无响应
✗ 触摸屏无法正确定位弹窗的关闭按钮
```

**根本原因：** LinuxFB 是单窗口设计，不支持 X11 的窗口管理

### 规范：固定尺寸 + 伪弹窗机制

#### ✅ 正确做法 - main.cpp

```cpp
#include <QFontDatabase>
#include <QScreen>

// ===== 全屏窗口设置 =====
void configureEmbeddedDisplay(QMainWindow *mainWin) {
    // 1. 设置固定尺寸（与硬件显示屏匹配）
    mainWin->setFixedSize(1024, 600);
    
    // 2. 移除所有窗口装饰（最大化、最小化、关闭按钮）
    mainWin->setWindowFlags(
        Qt::FramelessWindowHint      // 无边框
        | Qt::WindowStaysOnTopHint   // 始终在最前
    );
    
    // 3. 设置窗口位置（确保居中）
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - 1024) / 2;
    int y = (screenGeometry.height() - 600) / 2;
    mainWin->move(x, y);
    
    qDebug() << "[启动] 嵌入式窗口配置完成";
}

int main(int argc, char *argv[]) {
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    QApplication a(argc, argv);
    
    MainWindow w;
    configureEmbeddedDisplay(&w);  // 必须在 show() 之前调用
    w.show();
    
    return a.exec();
}
```

#### ✅ 正确做法 - 伪弹窗而不是 QDialog

```cpp
// ❌ 错误：使用独立的 QDialog
class SettingWindow : public QDialog {
    // ...
};

void MainWindow::on_pBtn_Setting_clicked() {
    settingWindow.show();  // 在硬件上可能卡死！
}

// ✅ 正确：使用 QWidget + show/hide + raise
class SettingWindow : public QWidget {
    // ... 作为一般的 Widget
};

void MainWindow::on_pBtn_Setting_clicked() {
    settingWindow.show();      // 显示设置窗口
    settingWindow.raise();     // 置顶到最前
    settingWindow.activateWindow();  // 获得焦点
}

void SettingWindow::on_pBtn_Close_clicked() {
    this->hide();  // 隐藏而非关闭，保持对象在内存中（便于下次快速显示）
}
```

#### 按钮尺寸规范（关联准则四）

```cpp
// ❌ 太小的按钮，在触摸屏上难以点击
QPushButton *btn = new QPushButton("确定");
btn->setFixedSize(30, 20);  // 触控死按钮！

// ✅ 最小尺寸 48x48 像素
QPushButton *btn = new QPushButton("确定");
btn->setFixedSize(60, 48);  // 友好的触摸热区
```

#### 检查清单

- [x] 主窗口设置为 `setFixedSize(1024, 600)`
- [x] 应用了 `Qt::FramelessWindowHint` 标记
- [x] 避免使用独立的 QDialog，改用伪弹窗
- [x] 所有可点击对象的尺寸 ≥ 48x48px
- [x] 确保 `configureEmbeddedDisplay()` 在 `show()` 前调用

---

## 准则三：内存约束与释放策略 🔴

### 现象与问题

```
❌ PC 开发（内存 16GB）：后台跑一礼拜没问题
✓ 内存占用稳定，很少看到 swap

❌ 硬件部署（内存 512MB）：运行几天就被系统强杀
✗ OOM Killer 强行关闭应用
✗ 日志观察：内存从 200MB 缓慢增长到 350MB，最后 OOM
✗ 导致整个车机死机，需要重启
```

**根本原因：** 三大内存泄漏陷阱
1. 图片资源频繁加载，没有及时释放
2. 网络请求数据（QByteArray / JSON）长期占用缓冲区
3. 多个页面同时实例化，闲置页面仍占内存

### 规范：克制资源，及时释放

#### [1] 图片加载与复用原则

```cpp
// ❌ 错误：频繁 new 和 delete 像素数据
for (int i = 0; i < 100; ++i) {
    QPixmap *pix = new QPixmap(":/images/icon.png");
    ui->label->setPixmap(*pix);
    delete pix;  // 频繁分配释放，造成内存碎片
}

// ✅ 正确：创建一次，复用多次
QPixmap iconPixmap(":/images/icon.png");  // 栈对象，自动管理
for (int i = 0; i < 100; ++i) {
    ui->labels[i]->setPixmap(iconPixmap);  // 直接复用
}

// ✅ 对于大尺寸背景图，使用 JPG 而不是 PNG
// PNG: 1024x600x4 = 2.4MB（内存占用）
// JPG: 1024x600, 质量 90% ≈ 0.5MB（内存占用）
QPixmap bgPixmap(":/images/background.jpg");  // 建议 JPG
```

#### [2] 网络请求数据及时释放

```cpp
void Weather::getWeatherInfo(QNetworkReply *reply) {
    // 解析 JSON...
    QByteArray bytes = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(bytes);
    
    // ... 提取关键数据 ...
    QString cityName = doc["city"].toString();
    
    // ===== 关键：解析完立即释放 reply =====
    reply->deleteLater();  // 删除网络对象，释放缓冲区
}

// ❌ 错误：长期持有 reply 指针
class Weather : public QMainWindow {
    QNetworkReply *lastReply;  // 危险！
    
    void getWeatherInfo(QNetworkReply *reply) {
        lastReply = reply;  // 长期占用内存
        // ...
    }
};
```

#### [3] 多页面懒加载策略（推荐）

```cpp
// ❌ 错误：全部在启动时创建
MainWindow::MainWindow() {
    mapWidget = new MapPage(this);      // 100MB 内存
    weatherWidget = new WeatherPage(this);  // 50MB 内存
    musicWidget = new MusicPage(this);     // 80MB 内存
    monitorWidget = new MonitorPage(this);  // 60MB 内存
    // 总计 290MB，而用户可能只用其中一个！
}

// ✅ 正确：懒加载（需要时才创建）
class MainWindow {
    MapPage *mapWidget = nullptr;       // 初始 nullptr
    WeatherPage *weatherWidget = nullptr;
    
public:
    void showMapPage() {
        if (!mapWidget) {
            mapWidget = new MapPage(this);  // 首次显示时创建
        }
        mapWidget->show();
    }
};

// ✅ 更优方案：使用 QStackedWidget（自动管理页面切换）
MainWindow::MainWindow() {
    stack = new QStackedWidget(this);
    stack->addWidget(new MapPage());      // 预加载（可选）
    stack->addWidget(new WeatherPage());
    stack->setCurrentIndex(0);
    
    connect(menu, &Menu::clicked, [this](int idx) {
        stack->setCurrentIndex(idx);  // 快速切换，无需释放重建
    });
}
```

#### 内存监控工具

```bash
# 在硬件上运行，监控内存占用
top -p $(pgrep VehicleTerminal)
# 或
cat /proc/[PID]/status | grep VmRSS

# 检测内存泄漏（可选：编译时添加 -fsanitize=address）
```

#### 检查清单

- [x] 网络请求完成后调用 `reply->deleteLater()`
- [x] 大尺寸图片优先使用 JPG 而不是 PNG
- [x] 避免频繁 new/delete 像素对象
- [x] 考虑使用 QStackedWidget 进行多页面管理
- [x] 闲置的大对象应该 hide() 而非持续占内存

---

## 准则四：字体与触控的硬件适配 ⚙️

### 现象与问题

```
❌ PC 上（Ubuntu 默认字体）：
✓ 文字大小刚好，清晰可读
✓ 默认字体支持中文

❌ 硬件上（嵌入式 Linux）：
✗ 文字极小，看不清楚
✗ 中文显示成豆腐块（不支持中文字体）
✗ 触控精准度低，按钮难以点击

原因：
1. PC 和硬件的默认字体完全不同
2. 硬件上可能没有安装微软雅黑等中文字体
3. 触摸屏 DPI 和精准度与鼠标差异大
```

### 规范：自带字体 + 大触摸热区

#### ✅ 正确做法 - 字体加载（main.cpp）

```cpp
#include <QFontDatabase>

void loadEmbeddedFonts() {
    // 1. 优先加载思源黑体（开源、跨平台、车载友好）
    int fontId = QFontDatabase::addApplicationFont(":/fonts/SourceHanSansCN-Bold.ttf");
    if (fontId != -1) {
        // 成功加载
        QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        qDebug() << "已加载字体：" << families;
    } else {
        // 备用字体
        qWarning() << "【警告】思源黑体加载失败，使用系统默认字体";
    }
    
    // 2. 设置全局应用字体
    QFont defaultFont("Source Han Sans CN", 12, QFont::Bold);
    qApp->setFont(defaultFont);
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    loadEmbeddedFonts();  // 必须在创建任何 Widget 之前
    
    MainWindow w;
    w.show();
    return a.exec();
}
```

#### 字体文件获取

```bash
# 思源黑体（推荐，开源）
# 下载地址：https://github.com/adobe-fonts/source-han-sans

# 放到项目目录
mkdir -p Fonts
cp SourceHanSansCN-Regular.ttf Fonts/
cp SourceHanSansCN-Bold.ttf Fonts/

# 添加到 .qrc
<qresource prefix="/fonts">
    <file>Fonts/SourceHanSansCN-Regular.ttf</file>
    <file>Fonts/SourceHanSansCN-Bold.ttf</file>
</qresource>
```

#### ✅ 正确做法 - 元素尺寸约定

```cpp
// ❌ 错误：按钮太小
btn_ok->setFixedSize(30, 20);  // 6mm × 4mm（对于72DPI屏幕）

// ✅ 正确：按钮最小 48×48px（约 17mm × 17mm，人类手指宽度）
btn_ok->setFixedSize(60, 48);
btn_cancel->setFixedSize(60, 48);

// ✅ 其他应遵循的尺寸规范
QLabel *label = new QLabel();
label->setMinimumHeight(36);  // 文本标签最小高度

QLineEdit *input = new QLineEdit();
input->setMinimumHeight(48);  // 输入框最小高度

QComboBox *combo = new QComboBox();
combo->setFixedHeight(48);   // 下拉框高度
```

#### ✅ 字号规范

```cpp
// 不要手硬编码字号，使用可扩展的方式
QFont titleFont;
titleFont.setPointSize(24);   // 标题：24pt
titleFont.setBold(true);

QFont contentFont;
contentFont.setPointSize(14);  // 正文：14pt

QFont labelFont;
labelFont.setPointSize(12);    // 标签：12pt
labelFont.setPixelSize(12);    // 或用 pixel 保证硬件一致性
```

#### ✅ 通过 QSS 声明字体（推荐）

```css
/* weather_dark_style.qss */
QLabel#label_now_tmp {
    font-family: "Source Han Sans CN", "Microsoft YaHei", sans-serif;
    font-size: 72px;
    font-weight: bold;
    color: #FFFFFF;
}

QLabel#label_now_des {
    font-family: "Source Han Sans CN", "Microsoft YaHei", sans-serif;
    font-size: 16px;
    color: #FFFFFF;
}

QPushButton {
    font-family: "Source Han Sans CN", "Microsoft YaHei", sans-serif;
    font-size: 14px;
    min-height: 48px;  /* 触摸热区 */
}
```

#### 检查清单

- [x] 在 main.cpp 中调用 `loadEmbeddedFonts()`
- [x] 将字体文件（.ttf）纳入 .qrc
- [x] 所有可点击对象的尺寸 ≥ 48×48px
- [x] 在 QSS 中指定 `font-family` 为自定义字体
- [x] 测试中文显示（不应有豆腐块）

---

## 快速检查清单 ✅

| 准则 | 项目 | 状态 |
|------|------|------|
| **一** | 所有 PNG/JPG/QSS/TTF 都在 .qrc | ✅ |
| **一** | 代码中使用 :/ 路径，无硬编码绝对路径 | ✅ |
| **二** | 主窗口固定尺寸 1024×600 | ✅ |
| **二** | 移除窗口装饰（无最大化/最小化按钮） | ✅ |
| **二** | 避免独立 QDialog，用伪弹窗 | ⚠️ (需检查) |
| **三** | 网络请求后调用 `reply->deleteLater()` | ✅ |
| **三** | 大图片使用 JPG 而不是 PNG | ⚠️ (需优化) |
| **四** | 在 main.cpp 中加载自定义字体 | ✅ |
| **四** | 所有按钮尺寸 ≥ 48×48px | ⚠️ (需检查) |
| **四** | QSS 中明确指定 font-family | ✅ |

---

## 部署流程（一句话检查表）

```bash
# 1. 编译资源
rcc img.qrc -o qrc_img.cpp

# 2. 编译应用
qmake -config release
make -j4

# 3. 复制到硬件（假设硬件 IP 为 192.168.1.100）
scp ./VehicleTerminal root@192.168.1.100:/app/bin/

# 4. 在硬件上运行
ssh root@192.168.1.100
cd /app/bin
./VehicleTerminal

# 5. 监控内存
top -p $(pidof VehicleTerminal)
cat /proc/$(pidof VehicleTerminal)/status | grep VmRSS
```

---

## 常见问题 FAQ

### Q1: 为什么我在 PC 上修改了字体，硬件上还是显示豆腐块？
**A:** 硬件上可能没有安装你使用的系统字体。必须使用 `QFontDatabase::addApplicationFont()` 加载自定义字体文件。

### Q2: 为什么硬件上图片全显示不出来？
**A:** 检查路径。应该使用 `:/weather/Weather/images/qing.png`，而不是 `/home/zeller/...`。

### Q3: 硬件运行一段时间后变卡，怎么办？
**A:** 十有八九是内存泄漏。检查网络请求是否及时释放 reply，大图片是否频繁加载。

### Q4: 触摸屏很难点中按钮，怎么解决？
**A:** 增大按钮尺寸至 ≥ 48×48px，考虑加入触摸反馈（蜂鸣音或振动）。

### Q5: 能否动态切换字体？
**A:** 可以，但没必要。使用 QSS 的 font-family fallback 就够了。例如：
```css
font-family: "Source Han Sans CN", "Microsoft YaHei", sans-serif;
```

---

## 参考资源

- Qt 资源系统文档：https://doc.qt.io/qt-5/resources.html
- LinuxFB 官方文档：https://doc.qt.io/qt-5/qwindowssysteminterface.html
- 思源黑体项目：https://github.com/adobe-fonts/source-han-sans
- i.MX6ULL 数据手册：https://www.nxp.com/products/processors/arm-processors/i-mx-series/i-mx-6-processors/i-mx6ull-processor

---

**最后提醒：** 这四项工程准则不仅是规范要求，也是面向 512MB 内存和无窗口管理器硬件环境的稳定性保障。严格执行这些约束，车机应用才能长期稳定运行。
