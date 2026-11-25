#pragma once
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QPointer>
class QNetworkReply;
class QLineEdit;
class QListWidget;
class QLabel;
class QPushButton;
class SearchMusic : public QMainWindow {
    Q_OBJECT
public:
    explicit SearchMusic(QWidget *parent = nullptr);
signals:
    void AddUrlMusic(QString name, QString path);

private:
    void search();
    QNetworkAccessManager network;
    QPointer<QNetworkReply> active;
    QLineEdit *query;
    QListWidget *results;
    QLabel *status;
    QPushButton *searchButton;
};
