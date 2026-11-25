#pragma once
#include <QMainWindow>
#include <QVector>
class WeatherService;
class QLabel;
class QLineEdit;
class QPushButton;
class Weather : public QMainWindow {
    Q_OBJECT
public:
    explicit Weather(WeatherService *service, QWidget *parent = nullptr);

private:
    void render();
    WeatherService *service;
    QLabel *temperature, *description, *city, *status, *image;
    QLineEdit *cityInput;
    QPushButton *refreshButton;
    QVector<QLabel *> dates, icons, temperatures, descriptions;
};
