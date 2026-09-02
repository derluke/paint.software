#include "desktopintegration.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QStringList>
#include <QVariant>

namespace {

QVariant unwrapVariant(QVariant value) {
    while (value.canConvert<QDBusVariant>())
        value = value.value<QDBusVariant>().variant();
    return value;
}

qreal sanitizedTextScale(const QVariant &value, bool *known) {
    bool ok = false;
    const qreal scale = unwrapVariant(value).toDouble(&ok);
    if (!ok || scale <= 0) return 1.0;
    *known = true;
    return qBound(0.5, scale, 3.0);
}

} // namespace

namespace DesktopIntegration {

bool isHyprlandSession(const QString &currentDesktop,
                       const QByteArray &hyprlandInstanceSignature) {
    if (!hyprlandInstanceSignature.trimmed().isEmpty()) return true;

    const QStringList desktops = currentDesktop.split(':', Qt::SkipEmptyParts);
    for (const QString &desktop : desktops) {
        if (desktop.trimmed().compare(QStringLiteral("Hyprland"),
                                      Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool prefersDockedUtilityWindows() {
    const QString override = qEnvironmentVariable("PAINTSW_UTILITY_LAYOUT")
                                 .trimmed().toLower();
    if (override == QStringLiteral("docked")) return true;
    if (override == QStringLiteral("floating")) return false;

    return isHyprlandSession(qEnvironmentVariable("XDG_CURRENT_DESKTOP"),
                             qgetenv("HYPRLAND_INSTANCE_SIGNATURE"));
}

TextScaleMonitor::TextScaleMonitor(QObject *parent) : QObject(parent) {
    QDBusConnection::sessionBus().connect(
        QString(),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("SettingChanged"),
        this,
        SLOT(handlePortalSettingChanged(QString,QString,QDBusVariant)));
    requestPortalTextScale();
}

void TextScaleMonitor::refresh() {
    requestPortalTextScale();
}

void TextScaleMonitor::requestPortalTextScale() {
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) return;

    QDBusMessage request = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.portal.Desktop"),
        QStringLiteral("/org/freedesktop/portal/desktop"),
        QStringLiteral("org.freedesktop.portal.Settings"),
        QStringLiteral("Read"));
    request << QStringLiteral("org.gnome.desktop.interface")
            << QStringLiteral("text-scaling-factor");

    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(request), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *finished) {
        const QDBusPendingReply<QDBusVariant> reply(*finished);
        finished->deleteLater();
        if (!reply.isValid()) return;

        bool known = false;
        const qreal scale = sanitizedTextScale(reply.value().variant(), &known);
        if (known) setTextScale(scale);
    });
}

void TextScaleMonitor::handlePortalSettingChanged(const QString &nameSpace,
                                                  const QString &key,
                                                  const QDBusVariant &value) {
    if (nameSpace != QStringLiteral("org.gnome.desktop.interface")
        || key != QStringLiteral("text-scaling-factor")) {
        return;
    }

    bool known = false;
    const qreal scale = sanitizedTextScale(value.variant(), &known);
    if (known) setTextScale(scale);
}

void TextScaleMonitor::setTextScale(qreal scale) {
    if (qFuzzyCompare(m_textScale, scale)) return;
    m_textScale = scale;
    emit textScaleChanged(m_textScale);
}

} // namespace DesktopIntegration
