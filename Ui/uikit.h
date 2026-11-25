#pragma once
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
class QAbstractButton;
class QTabWidget;
namespace UiKit {
QColor featureColor(const QString &feature);
QIcon icon(const QString &name, const QColor &color = QColor(), int size = 28);
QIcon badgeIcon(const QString &name, const QString &feature, int size = 36);
void setAccent(QWidget *widget, const QString &feature, bool tinted = false);
void setPrimary(QPushButton *button, const QString &feature);
void setButtonIcon(QAbstractButton *button, const QString &name, const QString &feature = QString(),
                   int size = 24, bool badge = false);
void setIcon(QLabel *label, const QString &name, const QString &feature, int size = 28,
             bool badge = false);
QLabel *iconLabel(const QString &name, const QString &feature, int size = 28, bool badge = false);
void setTabIcon(QTabWidget *tabs, int index, const QString &name, const QString &feature);
void refreshIcons(QWidget *root = nullptr);
void setStatus(QWidget *widget, const QString &status);
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
