#include "uikit.h"
#include "Core/i18n.h"
#include <QAbstractButton>
#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QTabWidget>
#include <QVariant>

namespace {
struct FeaturePalette {
    QString ink, tint, border, solid;
};
FeaturePalette palette(QString feature, bool night) {
    if (feature == "network")
        feature = "map";
    if (feature == "clock" || feature == "display")
        feature = "weather";
    if (feature == "climate" || feature == "humidity")
        feature = "camera";
    if (feature == "temperature")
        feature = "music";
    if (feature == "map")
        return night ? FeaturePalette{"#9cd6af", "#243d2f", "#48674e", "#2b7959"}
                     : FeaturePalette{"#2b7959", "#eaf5eb", "#c8dfcb", "#2b7959"};
    if (feature == "music")
        return night ? FeaturePalette{"#f1ac9a", "#442d29", "#795044", "#b64e42"}
                     : FeaturePalette{"#b64e42", "#fceee8", "#eccdc2", "#b64e42"};
    if (feature == "video")
        return night ? FeaturePalette{"#cdb2ee", "#342b44", "#65507c", "#7957a3"}
                     : FeaturePalette{"#7957a3", "#f2ecf9", "#dcd0ec", "#7957a3"};
    if (feature == "weather")
        return night ? FeaturePalette{"#eccb85", "#403721", "#746135", "#956719"}
                     : FeaturePalette{"#956719", "#fff5da", "#eddfb0", "#956719"};
    if (feature == "camera")
        return night ? FeaturePalette{"#95d4cd", "#203b3a", "#426b64", "#287d7c"}
                     : FeaturePalette{"#287d7c", "#e8f5f2", "#bedfda", "#287d7c"};
    if (feature == "settings")
        return night ? FeaturePalette{"#d1bca7", "#37302b", "#655343", "#7c695a"}
                     : FeaturePalette{"#7c695a", "#f1ece7", "#ded2c6", "#7c695a"};
    return night ? FeaturePalette{"#d4c2a8", "#343028", "#605442", "#75624b"}
                 : FeaturePalette{"#75624b", "#f7efe2", "#e6d8c2", "#75624b"};
}
QString iconFeature(const QString &name) {
    if (QStringList{"music", "heart", "play", "pause", "next", "previous", "volume", "repeat",
                    "shuffle"}
            .contains(name))
        return "music";
    if (name == "pin" || name == "map")
        return "map";
    if (name == "wifi")
        return "network";
    if (name == "mic")
        return "video";
    if (name == "clock" || name == "weather" || name == "brightness")
        return "weather";
    if (name == "temperature")
        return "temperature";
    if (name == "humidity")
        return "humidity";
    if (name == "camera")
        return "camera";
    if (name == "video")
        return "video";
    if (name == "settings" || name == "chip" || name == "storage" || name == "language")
        return "settings";
    return "home";
}
QString widgetFeature(QWidget *widget, const QString &name) {
    const QString explicitFeature = widget->property("uiIconFeature").toString();
    if (!explicitFeature.isEmpty())
        return explicitFeature;
    for (QWidget *ancestor = widget; ancestor; ancestor = ancestor->parentWidget()) {
        const QString feature = ancestor->property("accent").toString();
        if (!feature.isEmpty())
            return feature;
    }
    return iconFeature(name);
}
QString featureStyles(bool night) {
    QString css;
    const QStringList features = {"home",    "map",         "music",   "video", "weather",
                                  "camera",  "settings",    "network", "clock", "display",
                                  "climate", "temperature", "humidity"};
    for (const auto &feature : features) {
        const auto c = palette(feature, night);
        const QString attr = "[accent=\"" + feature + "\"]";
        const QString scope = "QWidget" + attr;
        css += "QPushButton" + attr + ":checked, QPushButton" + attr + ":pressed, " + scope +
               " QPushButton:checked, " + scope + " QPushButton:pressed {background:" + c.tint +
               ";color:" + c.ink + ";border-color:" + c.border + ";}\n";
        css += "QPushButton[nav=\"true\"]" + attr + ":checked {background:" + c.tint + ";}\n";
        css += "QPushButton[nav=\"true\"]" + attr + ":checked QLabel {color:" + c.ink + ";}\n";
        css += "QPushButton[quick=\"true\"]" + attr + " {background:" + c.tint +
               ";border-color:" + c.border + ";color:" + c.ink + ";}\n";
        css += "QPushButton[quick=\"true\"]" + attr + ":hover {border-color:" + c.ink + ";}\n";
        css += "QFrame#card" + attr + "[tinted=\"true\"], QFrame#heroCard" + attr +
               " {background:" + c.tint + ";border:1px solid " + c.border +
               ";border-radius:20px;}\n";
        css += "QLabel" + attr + " {color:" + c.ink + ";}\n";
        css += scope + " QListWidget::item:selected {background:" + c.tint + ";color:" + c.ink +
               ";}\n";
        css += scope + " QSlider::sub-page:horizontal, " + scope +
               " QSlider::handle:horizontal, QSlider" + attr + "::sub-page:horizontal, QSlider" +
               attr + "::handle:horizontal {background:" + c.ink + ";}\n";
        css += "QWidget QSlider" + attr + "::sub-page:horizontal:enabled, QWidget QSlider" + attr +
               "::handle:horizontal:enabled {background:" + c.ink + ";}\n";
        css += scope + " QLineEdit:focus {border-color:" + c.ink + ";}\n";
        css += scope + " QLineEdit {selection-background-color:" + c.tint +
               ";selection-color:" + c.ink + ";}\n";
        css += scope + " QTabBar::tab:selected {color:" + c.ink + ";border-bottom-color:" + c.ink +
               ";}\n";
        css += "QWidget QTabWidget" + attr + " QTabBar::tab:selected {color:" + c.ink +
               ";border-bottom-color:" + c.ink + ";}\n";
        css += scope + " QCheckBox::indicator:checked {background:" + c.ink + ";}\n";
        css += "QPushButton[primary=\"true\"]" + attr + ", " + scope +
               " QPushButton[primary=\"true\"] {background:" + c.solid +
               ";color:#ffffff;border:1px solid " + c.solid + ";}\n";
        css += "QPushButton[primary=\"true\"]" + attr + ":pressed, " + scope +
               " QPushButton[primary=\"true\"]:pressed {background:" +
               QColor(c.solid).darker(115).name() + ";}\n";
    }
    const auto green = palette("map", night);
    css += "QLabel[status=\"ready\"] {color:" + green.ink + ";}\n";
    css += "QLabel[role=\"badge\"][status=\"ready\"] {background:" + green.tint + ";}\n";
    css += night ? "QLabel[status=\"idle\"] {color:#b9b2a5;background:#34332e;}\n"
                 : "QLabel[status=\"idle\"] {color:#80756a;background:#f0ece5;}\n";
    css += night ? "QWidget QSlider::handle:horizontal:disabled {background:#70695e;}\n"
                 : "QWidget QSlider::handle:horizontal:disabled {background:#c3bdb2;}\n";
    return css;
}
} // namespace

QColor UiKit::featureColor(const QString &feature) {
    return QColor(palette(feature, qApp->property("nightTheme").toBool()).ink);
}

QIcon UiKit::icon(const QString &name, const QColor &color, int size) {
    QPixmap image(size * 2, size * 2);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(size / 12.0, size / 12.0);
    p.setPen(QPen(color.isValid() ? color : featureColor(iconFeature(name)), 1.7, Qt::SolidLine,
                  Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    if (name == "home") {
        path.moveTo(3, 11);
        path.lineTo(12, 3);
        path.lineTo(21, 11);
        path.moveTo(6, 10);
        path.lineTo(6, 21);
        path.lineTo(18, 21);
        path.lineTo(18, 10);
        path.moveTo(10, 21);
        path.lineTo(10, 15);
        path.lineTo(14, 15);
        path.lineTo(14, 21);
    } else if (name == "map") {
        path.moveTo(3, 5);
        path.lineTo(9, 3);
        path.lineTo(15, 5);
        path.lineTo(21, 3);
        path.lineTo(21, 19);
        path.lineTo(15, 21);
        path.lineTo(9, 19);
        path.lineTo(3, 21);
        path.closeSubpath();
        path.moveTo(9, 3);
        path.lineTo(9, 19);
        path.moveTo(15, 5);
        path.lineTo(15, 21);
    } else if (name == "music") {
        path.moveTo(9, 17);
        path.lineTo(9, 5);
        path.lineTo(20, 3);
        path.lineTo(20, 15);
        path.moveTo(9, 9);
        path.lineTo(20, 7);
        p.drawEllipse(QRectF(3, 16, 6, 5));
        p.drawEllipse(QRectF(14, 14, 6, 5));
    } else if (name == "video") {
        p.drawRoundedRect(QRectF(3, 5, 18, 14), 3, 3);
        path.moveTo(10, 9);
        path.lineTo(16, 12);
        path.lineTo(10, 15);
        path.closeSubpath();
    } else if (name == "camera") {
        p.drawRoundedRect(QRectF(2, 6, 14, 13), 3, 3);
        path.moveTo(16, 10);
        path.lineTo(22, 7);
        path.lineTo(22, 19);
        path.lineTo(16, 16);
    } else if (name == "weather") {
        p.drawEllipse(QRectF(7, 7, 10, 10));
        for (int i = 0; i < 8; ++i) {
            p.save();
            p.translate(12, 12);
            p.rotate(i * 45);
            p.drawLine(QPointF(0, -8), QPointF(0, -10));
            p.restore();
        }
    } else if (name == "settings") {
        p.drawEllipse(QRectF(5, 5, 14, 14));
        p.drawEllipse(QRectF(9, 9, 6, 6));
        for (int i = 0; i < 8; ++i) {
            p.save();
            p.translate(12, 12);
            p.rotate(i * 45);
            p.drawLine(0, -7, 0, -10);
            p.restore();
        }
    } else if (name == "play") {
        path.moveTo(8, 4);
        path.lineTo(20, 12);
        path.lineTo(8, 20);
        path.closeSubpath();
    } else if (name == "pause") {
        p.drawLine(8, 5, 8, 19);
        p.drawLine(16, 5, 16, 19);
    } else if (name == "next" || name == "previous") {
        if (name == "previous") {
            p.translate(24, 0);
            p.scale(-1, 1);
        }
        path.moveTo(5, 5);
        path.lineTo(16, 12);
        path.lineTo(5, 19);
        path.closeSubpath();
        p.drawLine(19, 5, 19, 19);
    } else if (name == "heart") {
        path.moveTo(12, 21);
        path.cubicTo(0, 13, 1, 3, 7, 4);
        path.cubicTo(10, 4, 12, 7, 12, 7);
        path.cubicTo(12, 7, 14, 4, 17, 4);
        path.cubicTo(23, 3, 24, 13, 12, 21);
    } else if (name == "wifi") {
        path.moveTo(2, 8);
        path.quadTo(12, -1, 22, 8);
        path.moveTo(6, 12);
        path.quadTo(12, 6, 18, 12);
        path.moveTo(10, 16);
        path.quadTo(12, 14, 14, 16);
        p.drawPoint(12, 20);
    } else if (name == "mic") {
        p.drawRoundedRect(QRectF(9, 2, 6, 13), 3, 3);
        path.moveTo(5, 10);
        path.cubicTo(5, 22, 19, 22, 19, 10);
        path.moveTo(12, 19);
        path.lineTo(12, 23);
    } else if (name == "back") {
        path.moveTo(14, 5);
        path.lineTo(7, 12);
        path.lineTo(14, 19);
    } else if (name == "refresh") {
        path.moveTo(20, 9);
        path.cubicTo(17, 0, 3, 2, 3, 12);
        path.cubicTo(3, 22, 18, 24, 21, 15);
        path.moveTo(20, 3);
        path.lineTo(20, 9);
        path.lineTo(14, 9);
    } else if (name == "volume") {
        path.moveTo(3, 9);
        path.lineTo(7, 9);
        path.lineTo(13, 4);
        path.lineTo(13, 20);
        path.lineTo(7, 15);
        path.lineTo(3, 15);
        path.closeSubpath();
        path.moveTo(17, 7);
        path.quadTo(23, 12, 17, 17);
    } else if (name == "pin") {
        path.moveTo(12, 22);
        path.cubicTo(-4, 5, 7, 1, 12, 2);
        path.cubicTo(17, 1, 28, 5, 12, 22);
        p.drawEllipse(QRectF(9, 6, 6, 6));
    } else if (name == "car") {
        p.drawRoundedRect(QRectF(3, 10, 18, 9), 3, 3);
        path.moveTo(5, 10);
        path.lineTo(8, 5);
        path.lineTo(16, 5);
        path.lineTo(19, 10);
        p.drawLine(6, 19, 6, 21);
        p.drawLine(18, 19, 18, 21);
        p.drawLine(6, 14, 8, 14);
        p.drawLine(16, 14, 18, 14);

    } else if (name == "temperature") {
        p.drawRoundedRect(QRectF(9, 2, 6, 13), 3, 3);
        p.drawEllipse(QRectF(7, 12, 10, 10));
        p.drawLine(12, 6, 12, 17);
    } else if (name == "humidity") {
        path.moveTo(12, 2);
        path.cubicTo(8, 8, 4, 11, 4, 16);
        path.cubicTo(4, 25, 20, 25, 20, 16);
        path.cubicTo(20, 11, 16, 8, 12, 2);
        p.drawLine(9, 16, 15, 11);
        p.drawPoint(9, 12);
        p.drawPoint(15, 16);
    } else if (name == "clock") {
        p.drawEllipse(QRectF(3, 3, 18, 18));
        path.moveTo(12, 6);
        path.lineTo(12, 12);
        path.lineTo(17, 15);
    } else if (name == "chip") {
        p.drawRoundedRect(QRectF(6, 6, 12, 12), 2, 2);
        p.drawRect(QRectF(9, 9, 6, 6));
        for (int i = 8; i <= 16; i += 4) {
            p.drawLine(i, 2, i, 6);
            p.drawLine(i, 18, i, 22);
            p.drawLine(2, i, 6, i);
            p.drawLine(18, i, 22, i);
        }
    } else if (name == "storage") {
        p.drawRoundedRect(QRectF(3, 5, 18, 15), 3, 3);
        p.drawLine(4, 14, 20, 14);
        p.drawPoint(17, 17);
        p.drawPoint(14, 17);
    } else if (name == "language") {
        p.drawEllipse(QRectF(3, 3, 18, 18));
        p.drawEllipse(QRectF(8, 3, 8, 18));
        p.drawLine(3, 12, 21, 12);
    } else if (name == "moon") {
        path.moveTo(17, 3);
        path.cubicTo(7, -1, -1, 12, 7, 19);
        path.cubicTo(12, 24, 22, 20, 22, 13);
        path.cubicTo(14, 17, 8, 9, 17, 3);
    } else if (name == "text") {
        p.drawLine(3, 5, 21, 5);
        p.drawLine(12, 5, 12, 21);
        p.drawLine(7, 21, 17, 21);
        p.drawLine(3, 5, 3, 9);
        p.drawLine(21, 5, 21, 9);
    } else if (name == "power") {
        path.moveTo(6, 5);
        path.cubicTo(-2, 12, 4, 23, 12, 22);
        path.cubicTo(22, 22, 27, 11, 18, 5);
        p.drawLine(12, 2, 12, 13);
    } else if (name == "search") {
        p.drawEllipse(QRectF(3, 3, 13, 13));
        p.drawLine(15, 15, 22, 22);
    } else if (name == "repeat") {
        path.moveTo(4, 8);
        path.lineTo(4, 5);
        path.lineTo(19, 5);
        path.lineTo(16, 2);
        path.moveTo(19, 5);
        path.lineTo(16, 8);
        path.moveTo(20, 16);
        path.lineTo(20, 19);
        path.lineTo(5, 19);
        path.lineTo(8, 22);
        path.moveTo(5, 19);
        path.lineTo(8, 16);
    } else if (name == "plus") {
        p.drawLine(4, 12, 20, 12);
        p.drawLine(12, 4, 12, 20);
    } else if (name == "minus")
        p.drawLine(4, 12, 20, 12);
    else {
        p.drawEllipse(QRectF(3, 3, 18, 18));
        p.drawLine(12, 7, 12, 13);
        p.drawPoint(12, 17);
    }
    p.drawPath(path);
    p.end();
    image.setDevicePixelRatio(2);
    return QIcon(image);
}
QLabel *UiKit::label(const QString &text, const char *role, QWidget *parent) {
    auto *item = new QLabel(parent);
    if (!text.isEmpty())
        I18n::setText(item, text);
    item->setProperty("role", role);
    item->setTextFormat(Qt::PlainText);
    return item;
}
QPushButton *UiKit::button(const QString &text, const QString &name, QWidget *parent) {
    auto *item = new QPushButton(parent);
    I18n::setText(item, text);
    item->setMinimumHeight(48);
    item->setCursor(Qt::PointingHandCursor);
    if (!name.isEmpty()) {
        setButtonIcon(item, name);
    }
    return item;
}
QFrame *UiKit::card(QWidget *parent) {
    auto *item = new QFrame(parent);
    item->setObjectName("card");
    return item;
}
void UiKit::applyTheme(bool night, bool largeText) {
    QFile file(":/ui/Ui/theme.qss");
    if (!file.open(QIODevice::ReadOnly))
        return;
    QString css = QString::fromUtf8(file.readAll());
    if (night) {
        css.replace("#f5f4f0", "#232622")
            .replace("#ffffff", "#30352f")
            .replace("#faf9f6", "#292d28")
            .replace("#e8e7e1", "#444b42")
            .replace("#262b26", "#eceee5")
            .replace("#7c8178", "#aeb6a8")
            .replace("#edf1e8", "#3a493b")
            .replace("#53665b", "#b5c9aa");
    }
    if (largeText)
        css.replace("15px", "17px").replace("13px", "15px");
    qApp->setProperty("nightTheme", night);
    qApp->setStyleSheet(css + featureStyles(night));
    refreshIcons();
}

QIcon UiKit::badgeIcon(const QString &name, const QString &feature, int size) {
    const auto colors = palette(feature, qApp->property("nightTheme").toBool());
    QPixmap image(size * 2, size * 2);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(2, 2);
    p.setPen(QPen(QColor(colors.border), 0.7));
    p.setBrush(QColor(colors.tint));
    p.drawRoundedRect(QRectF(0.5, 0.5, size - 1, size - 1), size * 0.3, size * 0.3);
    const int inset = qMax(4, size / 5);
    const int inner = size - inset * 2;
    icon(name, QColor(colors.ink), inner).paint(&p, QRect(inset, inset, inner, inner));
    p.end();
    image.setDevicePixelRatio(2);
    return QIcon(image);
}
void UiKit::setAccent(QWidget *widget, const QString &feature, bool tinted) {
    widget->setProperty("accent", feature);
    widget->setProperty("tinted", tinted);
    auto affected = widget->findChildren<QWidget *>();
    affected.prepend(widget);
    for (auto *item : affected) {
        item->style()->unpolish(item);
        item->style()->polish(item);
        item->update();
    }
    refreshIcons(widget);
}
void UiKit::setPrimary(QPushButton *button, const QString &feature) {
    button->setProperty("primary", true);
    setAccent(button, feature);
}
void UiKit::setButtonIcon(QAbstractButton *button, const QString &name, const QString &feature,
                          int size, bool badge) {
    button->setProperty("uiIconName", name);
    button->setProperty("uiIconFeature", feature);
    button->setProperty("uiIconSize", size);
    button->setProperty("uiIconBadge", badge);
    button->setIconSize(QSize(size, size));
    refreshIcons(button);
}
void UiKit::setIcon(QLabel *label, const QString &name, const QString &feature, int size,
                    bool badge) {
    label->setProperty("uiIconName", name);
    label->setProperty("uiIconFeature", feature);
    label->setProperty("uiIconSize", size);
    label->setProperty("uiIconBadge", badge);
    refreshIcons(label);
}
QLabel *UiKit::iconLabel(const QString &name, const QString &feature, int size, bool badge) {
    auto *item = label(QString());
    item->setAlignment(Qt::AlignCenter);
    item->setFixedSize(size, size);
    setIcon(item, name, feature, size, badge);
    return item;
}
void UiKit::setTabIcon(QTabWidget *tabs, int index, const QString &name, const QString &feature) {
    tabs->setProperty(("uiTabIcon" + QByteArray::number(index)).constData(),
                      QVariantMap{{"name", name}, {"feature", feature}});
    refreshIcons(tabs);
}
void UiKit::refreshIcons(QWidget *root) {
    QList<QWidget *> widgets = root ? root->findChildren<QWidget *>() : qApp->allWidgets();
    if (root)
        widgets.prepend(root);
    for (auto *widget : widgets) {
        const QString name = widget->property("uiIconName").toString();
        if (!name.isEmpty()) {
            const int size = widget->property("uiIconSize").toInt();
            const QString feature = widgetFeature(widget, name);
            const bool badge = widget->property("uiIconBadge").toBool();
            const QColor color =
                widget->property("primary").toBool() ? QColor("#ffffff") : featureColor(feature);
            const QIcon asset = badge ? badgeIcon(name, feature, size) : icon(name, color, size);
            if (auto *button = qobject_cast<QAbstractButton *>(widget))
                button->setIcon(asset);
            else if (auto *image = qobject_cast<QLabel *>(widget)) {
                I18n::instance().unbind(image, "text");
                image->setPixmap(asset.pixmap(size, size));
            }
        }
        if (auto *tabs = qobject_cast<QTabWidget *>(widget))
            for (int i = 0; i < tabs->count(); ++i) {
                const auto item =
                    tabs->property(("uiTabIcon" + QByteArray::number(i)).constData()).toMap();
                if (!item.isEmpty())
                    tabs->setTabIcon(i, badgeIcon(item.value("name").toString(),
                                                  item.value("feature").toString(), 24));
            }
    }
}
void UiKit::setStatus(QWidget *widget, const QString &status) {
    if (widget->property("status").toString() == status)
        return;
    widget->setProperty("status", status);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

void RoadArtwork::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(width() / 220.0, height() / 160.0);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#e4e9da"));
    p.drawEllipse(30, 5, 160, 145);
    QPainterPath road;
    road.moveTo(28, 170);
    road.cubicTo(40, 100, 180, 100, 163, -10);
    p.setPen(QPen(QColor("#fcfcf6"), 38, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(road);
    p.setPen(QPen(QColor("#abb99d"), 2, Qt::DashLine));
    p.drawPath(road);
    p.setPen(QPen(QColor("#fafaf4"), 2));
    p.setBrush(QColor("#75856b"));
    p.drawEllipse(QPointF(124, 94), 12, 12);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#fafaf4"));
    QPolygonF arrow;
    arrow << QPointF(124, 86) << QPointF(118, 100) << QPointF(124, 97) << QPointF(130, 100);
    p.drawPolygon(arrow);
}
