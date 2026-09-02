#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class QDBusVariant;

namespace DesktopIntegration {

// Kept as a pure helper so desktop detection can be covered without depending
// on the machine running the test suite.
bool isHyprlandSession(const QString &currentDesktop,
                       const QByteArray &hyprlandInstanceSignature);

// Tiling compositors should keep paint.software's four utility panels inside
// the main window. PAINTSW_UTILITY_LAYOUT=docked|floating is a useful escape
// hatch for users and automated integration tests.
bool prefersDockedUtilityWindows();

// Watches the desktop-wide apparent text size without blocking GUI startup.
// Omarchy drives this portal setting alongside its shell font-size override.
class TextScaleMonitor : public QObject {
    Q_OBJECT

public:
    explicit TextScaleMonitor(QObject *parent = nullptr);
    qreal textScale() const { return m_textScale; }

signals:
    void textScaleChanged(qreal scale);

public slots:
    void refresh();

private slots:
    void handlePortalSettingChanged(const QString &nameSpace, const QString &key,
                                    const QDBusVariant &value);

private:
    void requestPortalTextScale();
    void setTextScale(qreal scale);

    qreal m_textScale = 1.0;
};

} // namespace DesktopIntegration
