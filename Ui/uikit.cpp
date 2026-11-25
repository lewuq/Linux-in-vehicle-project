#include "uikit.h"
#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
QIcon UiKit::icon(const QString &name, const QColor &color, int size) {
    QPixmap image(size * 2, size * 2);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(size / 12.0, size / 12.0);
    p.setPen(QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
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
    auto *item = new QLabel(text, parent);
    item->setProperty("role", role);
    item->setTextFormat(Qt::PlainText);
    return item;
}
QPushButton *UiKit::button(const QString &text, const QString &name, QWidget *parent) {
    auto *item = new QPushButton(text, parent);
    item->setMinimumHeight(48);
    item->setCursor(Qt::PointingHandCursor);
    if (!name.isEmpty()) {
        item->setIcon(icon(name));
        item->setIconSize(QSize(24, 24));
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
    qApp->setStyleSheet(css);
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
