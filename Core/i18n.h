#pragma once
#include <QHash>
#include <QLocale>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QVector>

class QTabWidget;
class QWidget;
class QLabel;
class QPixmap;

// Localization belongs to presentation bindings; device data stays unchanged.
class I18n : public QObject {
    Q_OBJECT
public:
    static I18n &instance();
    void initialize();
    QString language() const {
        return currentLanguage;
    }
    QLocale locale() const;
    QString text(const QString &source) const;
    void setLanguage(const QString &language);
    void bind(QObject *object, const QByteArray &property, const QString &source);
    void unbind(QObject *object, const QByteArray &property);
    static void setText(QObject *object, const QString &source);
    static void setLiteralText(QObject *object, const QString &text);
    static void setPixmap(QLabel *label, const QPixmap &pixmap);
    static void setProperty(QObject *object, const char *property, const QString &source);
    static int addTab(QTabWidget *tabs, QWidget *page, const QString &source);
signals:
    void languageChanged();

private:
    explicit I18n(QObject *parent);
    QString render(const QString &source, int depth) const;
    void apply(QObject *object, const QByteArray &property, const QString &source);
    struct Format {
        QRegularExpression expression;
        QString translation;
        QVector<int> arguments;
        int specificity = 0;
        bool nestedStatus = false;
    };
    QString currentLanguage = QStringLiteral("zh_CN");
    QHash<QString, QString> english;
    QVector<Format> formats;
    QHash<QObject *, QHash<QByteArray, QString>> bindings;
};
