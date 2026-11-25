#include "weather.h"
#include "Core/weatherservice.h"
#include "Ui/uikit.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLocale>
#include <QRegularExpression>
#include <QVBoxLayout>
static QPixmap weatherIcon(const QString &name, int size) {
    QString safe = name;
    if (!QRegularExpression("^[a-z]+$").match(safe).hasMatch())
        safe = "yun";
    QPixmap image(":/weather/Weather/images/" + safe + ".png");
    return image.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}
Weather::Weather(WeatherService *weatherService, QWidget *parent)
    : QMainWindow(parent), service(weatherService) {
    setObjectName("page");
    auto *page = new QWidget;
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(8, 4, 8, 8);
    root->setSpacing(16);
    setCentralWidget(page);
    auto *header = new QHBoxLayout;
    header->addWidget(UiKit::label(QStringLiteral("沿途天气"), "title"));
    header->addStretch();
    cityInput = new QLineEdit;
    cityInput->setMaximumWidth(160);
    cityInput->setPlaceholderText(QStringLiteral("输入城市"));
    auto *change = UiKit::button(QStringLiteral("切换城市"), "pin");
    refreshButton = UiKit::button(QStringLiteral("刷新"), "refresh");
    header->addWidget(cityInput);
    header->addWidget(change);
    header->addWidget(refreshButton);
    root->addLayout(header);
    auto *hero = UiKit::card();
    hero->setObjectName("heroCard");
    auto *heroRow = new QHBoxLayout(hero);
    heroRow->setContentsMargins(30, 20, 30, 20);
    auto *info = new QVBoxLayout;
    city = UiKit::label(QStringLiteral("等待天气数据"), "heading");
    temperature = UiKit::label("--°", "hero");
    description = UiKit::label(QStringLiteral("暂无实况"), "body");
    info->addWidget(city);
    info->addWidget(temperature);
    info->addWidget(description);
    heroRow->addLayout(info, 1);
    image = UiKit::label(QString());
    image->setAlignment(Qt::AlignCenter);
    image->setPixmap(UiKit::icon("weather", QColor("#8a9b78"), 120).pixmap(120, 120));
    heroRow->addWidget(image, 1);
    root->addWidget(hero, 1);
    auto *forecast = new QHBoxLayout;
    forecast->setSpacing(10);
    for (int i = 0; i < 5; ++i) {
        auto *card = UiKit::card();
        auto *column = new QVBoxLayout(card);
        column->setContentsMargins(12, 14, 12, 14);
        dates << UiKit::label(i == 0 ? QStringLiteral("今天") : QStringLiteral("--"), "muted");
        icons << UiKit::label(QString());
        temperatures << UiKit::label("-- / --", "heading");
        descriptions << UiKit::label(QStringLiteral("暂无预报"), "muted");
        for (auto *item : {dates.last(), icons.last(), temperatures.last(), descriptions.last()}) {
            item->setAlignment(Qt::AlignCenter);
            column->addWidget(item);
        }
        forecast->addWidget(card, 1);
    }
    root->addLayout(forecast);
    status = UiKit::label(QString(), "muted");
    root->addWidget(status);
    connect(refreshButton, &QPushButton::clicked, service, &WeatherService::refresh);
    connect(change, &QPushButton::clicked, this, [this]() { service->setCity(cityInput->text()); });
    connect(cityInput, &QLineEdit::returnPressed, this,
            [this]() { service->setCity(cityInput->text()); });
    connect(service, &WeatherService::updated, this, &Weather::render);
    render();
    service->refresh();
}
void Weather::render() {
    const auto &report = service->report();
    status->setText(service->status());
    refreshButton->setEnabled(!service->busy());
    if (report.days.isEmpty()) {
        city->setText(QStringLiteral("等待天气数据"));
        temperature->setText("--°");
        description->setText(QStringLiteral("暂无实况"));
        for (int i = 0; i < dates.size(); ++i) {
            dates[i]->setText(i == 0 ? QStringLiteral("今天") : QStringLiteral("--"));
            icons[i]->clear();
            temperatures[i]->setText("-- / --");
            descriptions[i]->setText(QStringLiteral("暂无预报"));
        }
        return;
    }
    city->setText(report.city);
    const auto &today = report.days.first();
    temperature->setText(today.high + "°");
    description->setText(today.description + " · " + today.wind);
    image->setPixmap(weatherIcon(today.icon, 128));
    QLocale locale(QLocale::Chinese, QLocale::China);
    for (int i = 0; i < dates.size(); ++i) {
        if (i >= report.days.size()) {
            dates[i]->setText("--");
            icons[i]->clear();
            temperatures[i]->setText("-- / --");
            descriptions[i]->setText(QStringLiteral("暂无预报"));
            continue;
        }
        const auto &day = report.days[i];
        dates[i]->setText(i == 0 ? QStringLiteral("今天") : locale.toString(day.date, "ddd MM/dd"));
        icons[i]->setPixmap(weatherIcon(day.icon, 42));
        temperatures[i]->setText(day.high + "° / " + day.low + "°");
        descriptions[i]->setText(day.description);
    }
}
