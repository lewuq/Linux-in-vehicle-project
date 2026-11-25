#include "i18n.h"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QSettings>
#include <QTabWidget>
#include <QVariant>
#include <algorithm>

I18n &I18n::instance() {
    static I18n *service = new I18n(qApp);
    return *service;
}

I18n::I18n(QObject *parent) : QObject(parent) {
    QFile catalog(":/i18n/Translations/en_US.json");
    if (!catalog.open(QIODevice::ReadOnly))
        return;
    const auto entries = QJsonDocument::fromJson(catalog.readAll()).object();
    const QRegularExpression placeholder("%([1-9][0-9]*)");
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        english.insert(it.key(), it.value().toString());
        auto matches = placeholder.globalMatch(it.key());
        if (!matches.hasNext())
            continue;
        Format format;
        QString pattern = "^";
        int offset = 0;
        while (matches.hasNext()) {
            const auto match = matches.next();
            const QString literal = it.key().mid(offset, match.capturedStart() - offset);
            pattern += QRegularExpression::escape(literal) + "(.*?)";
            format.specificity += literal.size();
            format.arguments.append(match.captured(1).toInt());
            offset = match.capturedEnd();
        }
        const QString tail = it.key().mid(offset);
        pattern += QRegularExpression::escape(tail) + "$";
        format.specificity += tail.size();
        format.expression =
            QRegularExpression(pattern, QRegularExpression::DotMatchesEverythingOption);
        format.translation = it.value().toString();
        format.nestedStatus =
            it.key() == QStringLiteral("内存    %1") || it.key() == QStringLiteral("存储    %1") ||
            it.key() == QStringLiteral("运行时间    %1") || it.key().startsWith("WGS84");
        formats.append(format);
    }
    std::sort(formats.begin(), formats.end(),
              [](const Format &a, const Format &b) { return a.specificity > b.specificity; });
}

void I18n::initialize() {
    const QString saved = QSettings().value("display/language", "zh_CN").toString();
    currentLanguage = saved == "en_US" ? "en_US" : "zh_CN";
    QLocale::setDefault(locale());
}

QLocale I18n::locale() const {
    return QLocale(currentLanguage == "en_US" ? QLocale::English : QLocale::Chinese,
                   currentLanguage == "en_US" ? QLocale::UnitedStates : QLocale::China);
}

QString I18n::text(const QString &source) const {
    return currentLanguage == "en_US" ? render(source, 0) : source;
}

QString I18n::render(const QString &source, int depth) const {
    const auto exact = english.constFind(source);
    if (exact != english.cend())
        return exact.value();
    if (depth > 3)
        return source;
    for (const auto &format : formats) {
        const auto match = format.expression.match(source);
        if (!match.hasMatch())
            continue;
        QHash<int, QString> arguments;
        for (int i = 0; i < format.arguments.size(); ++i)
            arguments.insert(format.arguments[i], format.nestedStatus
                                                      ? render(match.captured(i + 1), depth + 1)
                                                      : match.captured(i + 1));
        // Replace placeholders in one pass so user data containing "%1" stays literal.
        QString translated;
        int offset = 0;
        const QRegularExpression placeholder("%([1-9][0-9]*)");
        auto matches = placeholder.globalMatch(format.translation);
        while (matches.hasNext()) {
            const auto item = matches.next();
            translated += format.translation.mid(offset, item.capturedStart() - offset);
            translated += arguments.value(item.captured(1).toInt(), item.captured());
            offset = item.capturedEnd();
        }
        translated += format.translation.mid(offset);
        return translated;
    }
    return source;
}

void I18n::apply(QObject *object, const QByteArray &property, const QString &source) {
    if (property.startsWith("tab:")) {
        auto *tabs = qobject_cast<QTabWidget *>(object);
        if (tabs)
            tabs->setTabText(property.mid(4).toInt(), text(source));
    } else {
        object->setProperty(property.constData(), text(source));
    }
}

void I18n::bind(QObject *object, const QByteArray &property, const QString &source) {
    if (!bindings.contains(object)) {
        connect(object, &QObject::destroyed, this, [this, object]() { bindings.remove(object); });
    }
    bindings[object].insert(property, source);
    apply(object, property, source);
}

void I18n::unbind(QObject *object, const QByteArray &property) {
    auto it = bindings.find(object);
    if (it != bindings.end())
        it->remove(property);
}

void I18n::setText(QObject *object, const QString &source) {
    instance().bind(object, "text", source);
}

void I18n::setLiteralText(QObject *object, const QString &value) {
    instance().unbind(object, "text");
    object->setProperty("text", value);
}

void I18n::setPixmap(QLabel *label, const QPixmap &pixmap) {
    instance().unbind(label, "text");
    label->setPixmap(pixmap);
}

void I18n::setProperty(QObject *object, const char *property, const QString &source) {
    instance().bind(object, property, source);
}

int I18n::addTab(QTabWidget *tabs, QWidget *page, const QString &source) {
    const int index = tabs->addTab(page, QString());
    instance().bind(tabs, "tab:" + QByteArray::number(index), source);
    return index;
}

void I18n::setLanguage(const QString &requested) {
    const QString language = requested == "en_US" ? "en_US" : "zh_CN";
    if (currentLanguage == language)
        return;
    currentLanguage = language;
    QSettings().setValue("display/language", language);
    QLocale::setDefault(locale());
    for (auto *widget : qApp->allWidgets())
        widget->setLocale(locale());
    for (auto it = bindings.begin(); it != bindings.end(); ++it)
        for (auto item = it.value().begin(); item != it.value().end(); ++item)
            apply(it.key(), item.key(), item.value());
    emit languageChanged();
}
