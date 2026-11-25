#include "weather.h"
#include "Core/i18n.h"
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
    UiKit::setAccent(this, "weather");
    auto *page = new QWidget;
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(8, 4, 8, 8);
    root->setSpacing(16);
    setCentralWidget(page);
    auto *header = new QHBoxLayout;
    header->addWidget(UiKit::iconLabel("weather", "weather", 34, true));
    header->addWidget(UiKit::label(QStringLiteral("沿途天气"), "title"));
    header->addStretch();
    cityInput = new QLineEdit;
    cityInput->setMaximumWidth(160);
    I18n::setProperty(cityInput, "placeholderText", QStringLiteral("输入城市"));
    auto *change = UiKit::button(QStringLiteral("切换城市"), "pin");
    refreshButton = UiKit::button(QStringLiteral("刷新"), "refresh");
    UiKit::setPrimary(refreshButton, "weather");
    header->addWidget(cityInput);
    header->addWidget(change);
    header->addWidget(refreshButton);
    root->addLayout(header);
    auto *hero = UiKit::card();
    hero->setObjectName("heroCard");
    UiKit::setAccent(hero, "weather", true);
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
    UiKit::setIcon(image, "weather", "weather", 110);
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
    connect(&I18n::instance(), &I18n::languageChanged, this, &Weather::render);
    render();
    service->refresh();
    UiKit::refreshIcons(this);
}
void Weather::render() {
    const auto &report = service->report();
    I18n::setText(status, service->status());
    refreshButton->setEnabled(!service->busy());
    if (report.days.isEmpty()) {
        I18n::setText(city, QStringLiteral("等待天气数据"));
        I18n::setText(temperature, "--°");
        I18n::setText(description, QStringLiteral("暂无实况"));
        for (int i = 0; i < dates.size(); ++i) {
            I18n::setText(dates[i], i == 0 ? QStringLiteral("今天") : QStringLiteral("--"));
            icons[i]->clear();
            I18n::setText(temperatures[i], "-- / --");
            I18n::setText(descriptions[i], QStringLiteral("暂无预报"));
        }
        return;
    }
    I18n::setLiteralText(city, report.city);
    const auto &today = report.days.first();
    I18n::setText(temperature, today.high + "°");
    I18n::setText(description, today.description + " · " + today.wind);
    image->setProperty("uiIconName", QString());
    I18n::setPixmap(image, weatherIcon(today.icon, 128));
    const QLocale locale = I18n::instance().locale();
    for (int i = 0; i < dates.size(); ++i) {
        if (i >= report.days.size()) {
            I18n::setText(dates[i], "--");
            icons[i]->clear();
            I18n::setText(temperatures[i], "-- / --");
            I18n::setText(descriptions[i], QStringLiteral("暂无预报"));
            continue;
        }
        const auto &day = report.days[i];
        I18n::setText(dates[i],
                      i == 0 ? QStringLiteral("今天") : locale.toString(day.date, "ddd MM/dd"));
        I18n::setPixmap(icons[i], weatherIcon(day.icon, 42));
        I18n::setText(temperatures[i], day.high + "° / " + day.low + "°");
        I18n::setText(descriptions[i], day.description);
    }
}
