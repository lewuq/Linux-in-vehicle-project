#pragma once
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
namespace UiKit {
QIcon icon(const QString &name, const QColor &color = QColor("#53665b"), int size = 28);
QLabel *label(const QString &text, const char *role = "body", QWidget *parent = nullptr);
QPushButton *button(const QString &text, const QString &iconName = QString(),
                    QWidget *parent = nullptr);
QFrame *card(QWidget *parent = nullptr);
void applyTheme(bool night = false, bool largeText = false);
} // namespace UiKit
class RoadArtwork : public QWidget {
public:
    explicit RoadArtwork(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumSize(140, 110);
    }

protected:
    void paintEvent(QPaintEvent *) override;
};
