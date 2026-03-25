#include "clock.h"
#include "ui_clock.h"

Clock::Clock(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Clock)
{
    ui->setupUi(this);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000);

    //设置窗体名称与大小
    setWindowTitle(tr("Clock"));
    setMinimumSize(160, 160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

Clock::~Clock()
{
    delete ui;
}

void Clock::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // 获取当前时间
    time = QTime::currentTime();

    int width = this->width();
    int height = this->height();
    int side = qMin(width, height) - 20;

    // 与主页卡片统一：圆角浅蓝灰底 + 细边框，避免内部直角割裂。
    painter.setPen(QPen(QColor(1, 9, 17, 60), 1));
    painter.setBrush(QColor(178, 209, 240, 250));
    painter.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 22, 22);

    // 统一坐标到圆心，并按窗口大小自适应。
    painter.save();
    painter.translate(width / 2, height / 2);
    painter.scale(side / 240.0, side / 240.0);

    // 表盘外圈。
    painter.setBrush(QColor("#eaf0f5"));
    painter.setPen(QPen(QColor("#c4d0db"), 2));
    painter.drawEllipse(QPoint(0, 0), 106, 106);

    // 绘制 60 个刻度：整点刻度更长更粗。
    for (int i = 0; i < 60; ++i) {
        painter.save();
        painter.rotate(i * 6.0);
        if (i % 5 == 0) {
            painter.setPen(QPen(QColor("#5a6f82"), 2));
            painter.drawLine(0, -92, 0, -80);
        } else {
            painter.setPen(QPen(QColor("#9fb0bf"), 1));
            painter.drawLine(0, -92, 0, -85);
        }
        painter.restore();
    }

    // 绘制 12 个数字。
    QFont numberFont("Roboto", 9, QFont::DemiBold);
    painter.setFont(numberFont);
    painter.setPen(QColor("#3c4f61"));
    for (int n = 1; n <= 12; ++n) {
        double angleDeg = n * 30.0 - 90.0;
        double angleRad = angleDeg * 3.14159265358979323846 / 180.0;
        int x = static_cast<int>(68 * cos(angleRad));
        int y = static_cast<int>(68 * sin(angleRad));
        QRect textRect(x - 10, y - 9, 20, 18);
        painter.drawText(textRect, Qt::AlignCenter, QString::number(n));
    }

    // 时针
    painter.save();
    painter.rotate(30.0 * (time.hour() + time.minute() / 60.0));
    painter.setPen(QPen(QColor("#2f4356"), 5, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(0, 6, 0, -42);
    painter.restore();

    // 分针
    painter.save();
    painter.rotate(6.0 * (time.minute() + time.second() / 60.0));
    painter.setPen(QPen(QColor("#3c5266"), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(0, 8, 0, -64);
    painter.restore();

    // 秒针
    painter.save();
    painter.rotate(6.0 * time.second());
    painter.setPen(QPen(QColor("#6f8faa"), 2, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(0, 10, 0, -72);
    painter.restore();

    // 中心轴点
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#2f4356"));
    painter.drawEllipse(-6, -6, 12, 12);
    painter.setBrush(QColor("#d8e2ea"));
    painter.drawEllipse(-2, -2, 4, 4);

    painter.restore();

    // 表盘下方的小号数字时间，作为辅助信息展示。
    QFont digitalFont("Roboto", 10, QFont::DemiBold);
    painter.setFont(digitalFont);
    painter.setPen(QColor("#6c7d8d"));
    QRect digitalRect(0, height - 32, width, 20);
    painter.drawText(digitalRect, Qt::AlignCenter, time.toString("hh:mm"));

}
