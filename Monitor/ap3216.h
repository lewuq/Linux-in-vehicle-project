/* Copyright © Deng Zhimao Co., Ltd. 1990-2030. All rights reserved.
 * Original sensor example, 2020-07-10; www.openedv.com. */
#pragma once
#include <QObject>
class Ap3216c : public QObject {
    Q_OBJECT
public:
    explicit Ap3216c(QObject *parent = nullptr) : QObject(parent) {}
    QString readPsData();
    QString readIrData();
    QString readAlsData();

private:
    QString readValue(const QString &name);
};
