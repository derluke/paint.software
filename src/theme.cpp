#include "theme.h"

#include <QSettings>
#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QPalette>
#include <QProcess>
#include <QRegularExpression>
#include <QScreen>
#include <QStyleHints>
#include <QTextStream>
#include <QVector>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

Theme::Scheme g_scheme = Theme::Scheme::Default;
QFont g_platformFont;
bool g_havePlatformFont = false;
qreal g_desktopTextScale = 1.0;

struct OmarchyPalette {
    bool valid = false;
    bool dark = true;
    int fontPixelSize = 12;
    bool userFontPixelSize = false;
    QString fontFamily = QStringLiteral("monospace");
    QColor accent;
    QColor selection;
    QColor muted;
    QColor background;
    QColor darkBackground;
    QColor darkerBackground;
    QColor lighterBackground;
    QColor foreground;
    QColor darkForeground;
    QColor lightForeground;
    QColor brightForeground;
    QColor red;
    QColor orange;
    QColor yellow;
    QColor green;
    QColor cyan;
    QColor blue;
    QColor purple;
    QColor magenta;
};

OmarchyPalette g_omarchy;

bool samePalette(const OmarchyPalette &a, const OmarchyPalette &b) {
    return a.valid == b.valid && a.dark == b.dark
        && a.fontPixelSize == b.fontPixelSize
        && a.userFontPixelSize == b.userFontPixelSize
        && a.fontFamily == b.fontFamily
        && a.accent == b.accent && a.selection == b.selection
        && a.muted == b.muted && a.background == b.background
        && a.darkBackground == b.darkBackground
        && a.darkerBackground == b.darkerBackground
        && a.lighterBackground == b.lighterBackground
        && a.foreground == b.foreground
        && a.darkForeground == b.darkForeground
        && a.lightForeground == b.lightForeground
        && a.brightForeground == b.brightForeground
        && a.red == b.red && a.orange == b.orange && a.yellow == b.yellow
        && a.green == b.green && a.cyan == b.cyan && a.blue == b.blue
        && a.purple == b.purple && a.magenta == b.magenta;
}

QString colorName(const QColor &color) {
    return color.name(color.alpha() == 255 ? QColor::HexRgb : QColor::HexArgb);
}

double relativeLuminance(const QColor &color) {
    auto channel = [](double v) {
        v /= 255.0;
        return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(color.red())
         + 0.7152 * channel(color.green())
         + 0.0722 * channel(color.blue());
}

double contrastRatio(const QColor &a, const QColor &b) {
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

QColor readableOn(const QColor &background, const QColor &light,
                  const QColor &dark) {
    return contrastRatio(background, light) >= contrastRatio(background, dark)
        ? light : dark;
}

// Ask the desktop directly. Returns 1 = dark, 0 = light, -1 = no answer.
// Needed because the Qt palette is not reliable everywhere: inside an AppImage
// the bundled Qt has no platform theme plugin, so its palette stays light even
// on a dark desktop, and the app came up light (issue #6).
int desktopSaysDark() {
    auto ask = [](const QString &schema, const QString &key) -> QString {
        QProcess p;
        p.start(QStringLiteral("gsettings"), {QStringLiteral("get"), schema, key});
        if (!p.waitForFinished(400)) { p.kill(); return {}; }
        if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) return {};
        return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    };

    // The freedesktop preference, when the desktop actually sets it.
    const QString pref = ask(QStringLiteral("org.gnome.desktop.interface"),
                             QStringLiteral("color-scheme"));
    if (pref.contains(QStringLiteral("prefer-dark"))) return 1;
    if (pref.contains(QStringLiteral("prefer-light"))) return 0;

    // Mint/Cinnamon (and others) leave color-scheme at 'default' and express the
    // choice purely through the GTK theme name, e.g. 'Mint-Y-Dark'.
    for (const QString &schema : {QStringLiteral("org.cinnamon.desktop.interface"),
                                  QStringLiteral("org.gnome.desktop.interface"),
                                  QStringLiteral("org.mate.interface")}) {
        const QString theme = ask(schema, QStringLiteral("gtk-theme"));
        if (!theme.isEmpty())
            return theme.contains(QStringLiteral("dark"), Qt::CaseInsensitive) ? 1 : 0;
    }

    const QString envTheme = qEnvironmentVariable("GTK_THEME");
    if (!envTheme.isEmpty())
        return envTheme.contains(QStringLiteral("dark"), Qt::CaseInsensitive) ? 1 : 0;

    return -1;
}

bool osPrefersDark() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (auto *hints = QGuiApplication::styleHints()) {
        const Qt::ColorScheme cs = hints->colorScheme();
        if (cs != Qt::ColorScheme::Unknown) return cs == Qt::ColorScheme::Dark;
    }
#endif
    // Cached: spawning gsettings is cheap but not free, and the desktop's choice
    // does not change mid-session in practice.
    static const int fromDesktop = desktopSaysDark();
    if (fromDesktop >= 0) return fromDesktop == 1;

    // Last resort: the palette the platform theme gave us.
    const QPalette pal = QApplication::palette();
    return pal.color(QPalette::Window).lightness() < 128;
}

// ---------------------------------------------------------------------------
// Light scheme — paint.net's default Windows light look: neutral grays, blue
// accent used only for hover/selection.
// ---------------------------------------------------------------------------
const char *kLight = R"(
    QMainWindow, QWidget { color: #1f1f1f; font-size: 11px; }
    QMainWindow { background-color: #f0f0f0; }
    QDialog { background-color: #f0f0f0; }

    QMenuBar { background-color: #f0f0f0; color: #1f1f1f; border-bottom: 1px solid #dcdcdc; font-size: 12px; min-height: 24px; }
    QMenuBar::item { padding: 4px 9px; background: transparent; }
    QMenuBar::item:selected { background-color: #cde4fa; }
    QMenuBar::item:pressed { background-color: #b6d8f5; }
    QMenu { background-color: #ffffff; color: #1f1f1f; border: 1px solid #c8c8c8; }
    QMenu::item { padding: 5px 26px 5px 26px; }
    QMenu::item:selected { background-color: #cde4fa; }
    QMenu::item:disabled { color: #a0a0a0; }
    QMenu::separator { height: 1px; background: #e0e0e0; margin: 3px 6px; }

    QToolBar#FixedToolbar { background-color: #f0f0f0; border-bottom: 1px solid #dcdcdc; spacing: 1px; padding: 1px 2px; min-height: 26px; }
    QToolBar#VariableToolbar { background-color: #f7f7f7; border-bottom: 1px solid #dcdcdc; spacing: 2px; padding: 1px 2px; min-height: 28px; }
    QToolBar::separator { background: #d4d4d4; width: 1px; margin: 3px 3px; }

    QToolButton { color: #1f1f1f; background-color: transparent; border: 1px solid transparent; border-radius: 3px; padding: 2px; }
    QToolButton:hover { background-color: #e3eff9; border-color: #a8cdea; }
    QToolButton:checked { background-color: #cde4fa; border-color: #7ab0dd; }
    QToolButton:pressed { background-color: #b6d8f5; }

    QDockWidget { font-size: 11px; color: #1f1f1f; background-color: #f0f0f0; }
    QDockWidget::title { background-color: #e4e4e4; border: 1px solid #cfcfcf; padding: 4px 6px; color: #333333; font-weight: bold; text-align: left; }
    #ToolsPalette, LayersPanel, HistoryPanel, ToolOptionsPanel { background-color: #f0f0f0; }
    /* paint.net's Colors window is white, like the Layers/History list areas. */
    ColorsPanel { background-color: #ffffff; }
    QDockWidget::close-button, QDockWidget::float-button {
        subcontrol-position: top right; subcontrol-origin: margin;
        background: transparent; border: 1px solid transparent;
        border-radius: 2px; width: 14px; height: 14px; top: 2px;
    }
    QDockWidget::close-button { right: 3px; }
    QDockWidget::float-button { right: 19px; }
    QDockWidget::close-button:hover { background: #e8a0a0; border-color: #d08080; }
    QDockWidget::float-button:hover { background: #cde4fa; border-color: #7ab0dd; }

    QLabel { color: #1f1f1f; }
    QGroupBox { border: 1px solid #d0d0d0; border-radius: 3px; margin-top: 8px; padding-top: 6px; }
    QGroupBox::title { subcontrol-origin: margin; left: 7px; padding: 0 3px; }

    QSlider::groove:horizontal { height: 4px; background: #d0d0d0; border-radius: 2px; }
    QSlider::sub-page:horizontal { background: #7ab0dd; border-radius: 2px; }
    QSlider::handle:horizontal { background: #fdfdfd; border: 1px solid #8a8a8a; width: 9px; margin: -5px 0; border-radius: 2px; }
    QSlider::handle:horizontal:hover { border-color: #4a90d9; }

    QListWidget { background-color: #ffffff; color: #1f1f1f; border: 1px solid #c8c8c8; }
    QListWidget::item { padding: 2px; }
    QListWidget::item:selected { background-color: #cde4fa; color: #1f1f1f; }

    QPushButton { background-color: #f0f0f0; color: #1f1f1f; border: 1px solid #b4b4b4; padding: 3px 7px; border-radius: 3px; font-size: 11px; }
    QPushButton:hover { background-color: #e3eff9; border-color: #a8cdea; }
    QPushButton:pressed { background-color: #cde4fa; }
    QPushButton:checked { background-color: #cde4fa; border-color: #7ab0dd; }

    QSpinBox, QComboBox, QLineEdit { background-color: #ffffff; color: #1f1f1f; border: 1px solid #b4b4b4; padding: 1px 3px; border-radius: 2px; font-size: 11px; }
    QSpinBox:focus, QComboBox:focus, QLineEdit:focus { border-color: #4a90d9; }
    QComboBox::drop-down { border: none; width: 16px; }
    QComboBox QAbstractItemView { background-color: #ffffff; color: #1f1f1f; selection-background-color: #cde4fa; selection-color: #1f1f1f; }
    QCheckBox { color: #1f1f1f; font-size: 11px; spacing: 4px; }

    QTabWidget::pane { border: 1px solid #c8c8c8; background: #f7f7f7; }
    QTabBar::tab { background: #e6e6e6; color: #1f1f1f; padding: 5px 12px; border: 1px solid #c8c8c8; border-bottom: none; }
    QTabBar::tab:selected { background: #f7f7f7; }

    QStatusBar { background-color: #f0f0f0; color: #333; border-top: 1px solid #dcdcdc; font-size: 11px; }
    QStatusBar QLabel { color: #333; padding: 0 3px; }
    QStatusBar::item { border: none; }

    QScrollBar:vertical { background: #f4f4f4; width: 14px; margin: 0; }
    QScrollBar::handle:vertical { background: #c6c6c6; border-radius: 3px; min-height: 24px; }
    QScrollBar::handle:vertical:hover { background: #a8a8a8; }
    QScrollBar:horizontal { background: #f4f4f4; height: 14px; margin: 0; }
    QScrollBar::handle:horizontal { background: #c6c6c6; border-radius: 3px; min-width: 24px; }
    QScrollBar::handle:horizontal:hover { background: #a8a8a8; }
    QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
)";

// ---------------------------------------------------------------------------
// Dark scheme — paint.net's dark color scheme: near-black chrome, #2b2b2b
// surfaces, light gray text, same blue accent.
// ---------------------------------------------------------------------------
const char *kDark = R"(
    QMainWindow, QWidget { color: #f0f0f0; font-size: 11px; }
    QMainWindow { background-color: #2b2b2b; }
    QDialog { background-color: #2b2b2b; }

    QMenuBar { background-color: #2b2b2b; color: #f0f0f0; border-bottom: 1px solid #1e1e1e; font-size: 12px; min-height: 24px; }
    QMenuBar::item { padding: 4px 9px; background: transparent; }
    QMenuBar::item:selected { background-color: #3f5f7f; }
    QMenuBar::item:pressed { background-color: #2d6ba3; }
    QMenu { background-color: #333333; color: #f0f0f0; border: 1px solid #4a4a4a; }
    QMenu::item { padding: 5px 26px 5px 26px; }
    QMenu::item:selected { background-color: #2d6ba3; }
    QMenu::item:disabled { color: #7a7a7a; }
    QMenu::separator { height: 1px; background: #4a4a4a; margin: 3px 6px; }

    QToolBar#FixedToolbar { background-color: #2b2b2b; border-bottom: 1px solid #1e1e1e; spacing: 1px; padding: 1px 2px; min-height: 26px; }
    QToolBar#VariableToolbar { background-color: #323232; border-bottom: 1px solid #1e1e1e; spacing: 2px; padding: 1px 2px; min-height: 28px; }
    QToolBar::separator { background: #4a4a4a; width: 1px; margin: 3px 3px; }

    QToolButton { color: #f0f0f0; background-color: transparent; border: 1px solid transparent; border-radius: 3px; padding: 2px; }
    QToolButton:hover { background-color: #3d3d3d; border-color: #5a5a5a; }
    QToolButton:checked { background-color: #2d6ba3; border-color: #4a90d9; }
    QToolButton:pressed { background-color: #2d6ba3; }

    QDockWidget { font-size: 11px; color: #f0f0f0; background-color: #2b2b2b; }
    QDockWidget::title { background-color: #3a3a3a; border: 1px solid #4a4a4a; padding: 4px 6px; color: #e8e8e8; font-weight: bold; text-align: left; }
    #ToolsPalette, LayersPanel, ColorsPanel, HistoryPanel, ToolOptionsPanel { background-color: #2b2b2b; }
    QDockWidget::close-button, QDockWidget::float-button {
        subcontrol-position: top right; subcontrol-origin: margin;
        background: transparent; border: 1px solid transparent;
        border-radius: 2px; width: 14px; height: 14px; top: 2px;
    }
    QDockWidget::close-button { right: 3px; }
    QDockWidget::float-button { right: 19px; }
    QDockWidget::close-button:hover { background: #a04040; border-color: #c05050; }
    QDockWidget::float-button:hover { background: #2d6ba3; border-color: #4a90d9; }

    QLabel { color: #f0f0f0; }
    QGroupBox { border: 1px solid #4a4a4a; border-radius: 3px; margin-top: 8px; padding-top: 6px; }
    QGroupBox::title { subcontrol-origin: margin; left: 7px; padding: 0 3px; }

    QSlider::groove:horizontal { height: 4px; background: #4a4a4a; border-radius: 2px; }
    QSlider::sub-page:horizontal { background: #4a90d9; border-radius: 2px; }
    QSlider::handle:horizontal { background: #d0d0d0; border: 1px solid #6a6a6a; width: 9px; margin: -5px 0; border-radius: 2px; }
    QSlider::handle:horizontal:hover { border-color: #4a90d9; }

    QListWidget { background-color: #1e1e1e; color: #f0f0f0; border: 1px solid #4a4a4a; }
    QListWidget::item { padding: 2px; }
    QListWidget::item:selected { background-color: #2d6ba3; color: #ffffff; }

    /* File dialogs draw their file list with QTreeView/QListView and a QHeaderView.
       Those were unstyled, so they kept a white background while the global rule
       made the text light — white on white, unreadable (issue #7). */
    QAbstractItemView, QTreeView, QListView, QTableView, QColumnView {
        background-color: #1e1e1e; alternate-background-color: #232323; color: #f0f0f0;
        border: 1px solid #4a4a4a; selection-background-color: #2d6ba3; selection-color: #ffffff;
    }
    QTreeView::item:hover, QListView::item:hover { background-color: #333333; }
    QHeaderView::section {
        background-color: #2b2b2b; color: #f0f0f0;
        border: 1px solid #4a4a4a; padding: 3px 5px;
    }
    QTableCornerButton::section { background-color: #2b2b2b; border: 1px solid #4a4a4a; }

    QPushButton { background-color: #3a3a3a; color: #f0f0f0; border: 1px solid #5a5a5a; padding: 3px 7px; border-radius: 3px; font-size: 11px; }
    QPushButton:hover { background-color: #464646; border-color: #6a6a6a; }
    QPushButton:pressed { background-color: #2d6ba3; }
    QPushButton:checked { background-color: #2d6ba3; border-color: #4a90d9; }

    QSpinBox, QDoubleSpinBox, QComboBox, QLineEdit { background-color: #1e1e1e; color: #f0f0f0; border: 1px solid #5a5a5a; padding: 1px 3px; border-radius: 2px; font-size: 11px; }
    QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QLineEdit:focus { border-color: #4a90d9; }
    /* The native up/down arrows are dark and vanish on the dark field (issue #11).
       Qt's stylesheet fills a border-triangle as a solid block, so use light
       arrow images instead. */
    QAbstractSpinBox::up-button, QAbstractSpinBox::down-button { background: #333333; border-left: 1px solid #5a5a5a; width: 15px; }
    QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover { background: #454545; }
    QAbstractSpinBox::up-arrow { image: url(:/spin-up.png); width: 8px; height: 8px; }
    QAbstractSpinBox::down-arrow { image: url(:/spin-down.png); width: 8px; height: 8px; }
    QComboBox::drop-down { border: none; width: 16px; }
    QComboBox::down-arrow { image: url(:/spin-down.png); width: 8px; height: 8px; }
    QComboBox QAbstractItemView { background-color: #1e1e1e; color: #f0f0f0; selection-background-color: #2d6ba3; selection-color: #ffffff; }
    QCheckBox { color: #f0f0f0; font-size: 11px; spacing: 4px; }

    QTabWidget::pane { border: 1px solid #4a4a4a; background: #323232; }
    QTabBar::tab { background: #2b2b2b; color: #f0f0f0; padding: 5px 12px; border: 1px solid #4a4a4a; border-bottom: none; }
    QTabBar::tab:selected { background: #323232; }

    QStatusBar { background-color: #2b2b2b; color: #d0d0d0; border-top: 1px solid #1e1e1e; font-size: 11px; }
    QStatusBar QLabel { color: #d0d0d0; padding: 0 3px; }
    QStatusBar::item { border: none; }

    QScrollBar:vertical { background: #2b2b2b; width: 14px; margin: 0; }
    QScrollBar::handle:vertical { background: #555555; border-radius: 3px; min-height: 24px; }
    QScrollBar::handle:vertical:hover { background: #6a6a6a; }
    QScrollBar:horizontal { background: #2b2b2b; height: 14px; margin: 0; }
    QScrollBar::handle:horizontal { background: #555555; border-radius: 3px; min-width: 24px; }
    QScrollBar::handle:horizontal:hover { background: #6a6a6a; }
    QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
)";

// Omarchy themes share a compact semantic palette. Keeping the stylesheet in
// those terms means paint.software follows every built-in and user theme rather
// than trying to recognise theme names.
const char *kOmarchy = R"(
    QMainWindow, QWidget { color: @foreground@; font-size: 12px; }
    QMainWindow, QDialog { background-color: @background@; }
    QToolTip { background-color: @lighter@; color: @foreground@; border: 1px solid @muted@; padding: 3px; }

    QMenuBar { background-color: @background@; color: @foreground@; border-bottom: 1px solid @darker@; font-size: 12px; min-height: 24px; }
    QMenuBar::item { padding: 4px 9px; background: transparent; }
    QMenuBar::item:selected, QMenuBar::item:pressed { background-color: @selection@; }
    QMenu { background-color: @lighter@; color: @foreground@; border: 1px solid @muted@; }
    QMenu::item { padding: 5px 26px; }
    QMenu::item:selected { background-color: @accent@; color: @onAccent@; }
    QMenu::item:disabled { color: @disabled@; }
    QMenu::separator { height: 1px; background: @muted@; margin: 3px 6px; }

    QToolBar#FixedToolbar { background-color: @background@; border-bottom: 1px solid @darker@; spacing: 1px; padding: 1px 2px; min-height: 26px; }
    QToolBar#VariableToolbar { background-color: @lighter@; border-bottom: 1px solid @darker@; spacing: 2px; padding: 1px 2px; min-height: 28px; }
    QToolBar::separator { background: @muted@; width: 1px; margin: 3px; }
    QToolButton { color: @foreground@; background-color: transparent; border: 1px solid transparent; border-radius: 3px; padding: 2px; }
    QToolButton:hover { background-color: @selection@; border-color: @muted@; }
    QToolButton:checked, QToolButton:pressed { background-color: @accent@; color: @onAccent@; border-color: @accent@; }
    QToolButton:disabled { color: @disabled@; }

    QDockWidget { color: @foreground@; background-color: @background@; }
    QDockWidget::title { background-color: @lighter@; border: 1px solid @muted@; padding: 4px 6px; color: @bright@; font-weight: bold; text-align: left; }
    #ToolsPalette, LayersPanel, ColorsPanel, HistoryPanel, ToolOptionsPanel { background-color: @background@; }
    QDockWidget::close-button, QDockWidget::float-button { subcontrol-position: top right; subcontrol-origin: margin; background: transparent; border: 1px solid transparent; border-radius: 2px; width: 14px; height: 14px; top: 2px; }
    QDockWidget::close-button { right: 3px; }
    QDockWidget::float-button { right: 19px; }
    QDockWidget::close-button:hover { background: @red@; border-color: @red@; }
    QDockWidget::float-button:hover { background: @selection@; border-color: @accent@; }

    QLabel { color: @foreground@; }
    QGroupBox { border: 1px solid @muted@; border-radius: 3px; margin-top: 8px; padding-top: 6px; }
    QGroupBox::title { subcontrol-origin: margin; left: 7px; padding: 0 3px; }
    QSlider::groove:horizontal { height: 4px; background: @muted@; border-radius: 2px; }
    QSlider::sub-page:horizontal { background: @accent@; border-radius: 2px; }
    QSlider::handle:horizontal { background: @bright@; border: 1px solid @muted@; width: 9px; margin: -5px 0; border-radius: 2px; }
    QSlider::handle:horizontal:hover { border-color: @accent@; }

    QAbstractItemView, QListWidget, QTreeView, QListView, QTableView, QColumnView {
        background-color: @dark@; alternate-background-color: @darker@; color: @foreground@;
        border: 1px solid @muted@; selection-background-color: @accent@; selection-color: @onAccent@;
    }
    QListWidget::item { padding: 2px; }
    QTreeView::item:hover, QListView::item:hover { background-color: @selection@; }
    QHeaderView::section { background-color: @background@; color: @foreground@; border: 1px solid @muted@; padding: 3px 5px; }
    QTableCornerButton::section { background-color: @background@; border: 1px solid @muted@; }

    QPushButton { background-color: @lighter@; color: @foreground@; border: 1px solid @muted@; padding: 3px 7px; border-radius: 3px; }
    QPushButton:hover { background-color: @selection@; border-color: @accent@; }
    QPushButton:pressed, QPushButton:checked { background-color: @accent@; color: @onAccent@; border-color: @accent@; }
    QPushButton:disabled { color: @disabled@; }

    QSpinBox, QDoubleSpinBox, QComboBox, QLineEdit { background-color: @dark@; color: @foreground@; border: 1px solid @muted@; padding: 1px 3px; border-radius: 2px; }
    QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QLineEdit:focus { border-color: @accent@; }
    QAbstractSpinBox::up-button, QAbstractSpinBox::down-button { background: @lighter@; border-left: 1px solid @muted@; width: 15px; }
    QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover { background: @selection@; }
    QComboBox::drop-down { border: none; width: 16px; }
    QComboBox QAbstractItemView { background-color: @dark@; color: @foreground@; selection-background-color: @accent@; selection-color: @onAccent@; }
    QCheckBox { color: @foreground@; spacing: 4px; }

    QTabWidget::pane { border: 1px solid @muted@; background: @lighter@; }
    QTabBar::tab { background: @background@; color: @foreground@; padding: 5px 12px; border: 1px solid @muted@; border-bottom: none; }
    QTabBar::tab:selected { background: @lighter@; border-color: @accent@; }
    QStatusBar { background-color: @background@; color: @lightForeground@; border-top: 1px solid @darker@; }
    QStatusBar QLabel { color: @lightForeground@; padding: 0 3px; }
    QStatusBar::item { border: none; }

    QScrollBar:vertical { background: @background@; width: 14px; margin: 0; }
    QScrollBar::handle:vertical { background: @muted@; border-radius: 3px; min-height: 24px; }
    QScrollBar::handle:vertical:hover { background: @accent@; }
    QScrollBar:horizontal { background: @background@; height: 14px; margin: 0; }
    QScrollBar::handle:horizontal { background: @muted@; border-radius: 3px; min-width: 24px; }
    QScrollBar::handle:horizontal:hover { background: @accent@; }
    QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
)";

bool readFontBaseSize(const QString &path, int *size) {
    QFile shellFile(path);
    if (!shellFile.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream input(&shellFile);
    bool inFontSection = false;
    const QRegularExpression section(QStringLiteral(R"(^\s*\[([^]]+)\])"));
    const QRegularExpression baseSize(
        QStringLiteral(R"(^\s*base-size\s*=\s*([0-9]+))"));
    while (!input.atEnd()) {
        const QString line = input.readLine();
        const QRegularExpressionMatch sectionMatch = section.match(line);
        if (sectionMatch.hasMatch()) {
            inFontSection = sectionMatch.captured(1).trimmed()
                == QStringLiteral("font");
            continue;
        }
        if (!inFontSection) continue;

        const QRegularExpressionMatch sizeMatch = baseSize.match(line);
        if (!sizeMatch.hasMatch()) continue;
        *size = std::max(1, sizeMatch.captured(1).toInt());
        return true;
    }
    return false;
}

OmarchyPalette readOmarchyPalette(const QString &path) {
    OmarchyPalette result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return result;

    QHash<QString, QString> values;
    QTextStream input(&file);
    const QRegularExpression assignment(
        QStringLiteral(R"(^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*["']([^"']+)["'])"));
    while (!input.atEnd()) {
        const QRegularExpressionMatch match = assignment.match(input.readLine());
        if (match.hasMatch()) values.insert(match.captured(1), match.captured(2));
    }

    auto requiredColor = [&values](const QString &key, QColor &target) {
        target = QColor(values.value(key));
        return target.isValid();
    };
    const QString mode = values.value(QStringLiteral("mode")).trimmed().toLower();
    if (mode != QStringLiteral("dark") && mode != QStringLiteral("light")) return result;
    result.dark = mode == QStringLiteral("dark");

    if (!requiredColor(QStringLiteral("accent"), result.accent)
        || !requiredColor(QStringLiteral("selection"), result.selection)
        || !requiredColor(QStringLiteral("muted"), result.muted)
        || !requiredColor(QStringLiteral("background"), result.background)
        || !requiredColor(QStringLiteral("dark_background"), result.darkBackground)
        || !requiredColor(QStringLiteral("darker_background"), result.darkerBackground)
        || !requiredColor(QStringLiteral("lighter_background"), result.lighterBackground)
        || !requiredColor(QStringLiteral("foreground"), result.foreground)
        || !requiredColor(QStringLiteral("dark_foreground"), result.darkForeground)
        || !requiredColor(QStringLiteral("light_foreground"), result.lightForeground)
        || !requiredColor(QStringLiteral("bright_foreground"), result.brightForeground)) {
        return OmarchyPalette{};
    }

    auto optionalColor = [&values](const QString &key, const QColor &fallback) {
        const QColor color(values.value(key));
        return color.isValid() ? color : fallback;
    };
    result.red = optionalColor(QStringLiteral("red"), QColor(QStringLiteral("#d75f5f")));
    result.yellow = optionalColor(QStringLiteral("yellow"), result.accent);
    result.orange = optionalColor(QStringLiteral("orange"), result.yellow);
    result.green = optionalColor(QStringLiteral("green"), result.accent);
    result.cyan = optionalColor(QStringLiteral("cyan"), result.accent);
    result.blue = optionalColor(QStringLiteral("blue"), result.accent);
    result.magenta = optionalColor(QStringLiteral("magenta"), result.accent);
    result.purple = optionalColor(QStringLiteral("purple"), result.magenta);

    // Theme shell.toml supplies the default type rhythm. `omarchy display text
    // size` writes a machine-level override to ~/.config/omarchy/shell.toml;
    // layer that on top so the choice survives theme switches just as it does
    // in the Omarchy shell itself.
    readFontBaseSize(QFileInfo(path).dir().filePath(QStringLiteral("shell.toml")),
                     &result.fontPixelSize);

    // A palette override is normally an isolated test/custom setup and should
    // not accidentally inherit the real machine's user settings. Tests can opt
    // back in with PAINTSW_OMARCHY_USER_SHELL.
    const bool paletteOverridden =
        !qEnvironmentVariable("PAINTSW_OMARCHY_COLORS").trimmed().isEmpty();
    const bool userShellOverridden =
        !qEnvironmentVariable("PAINTSW_OMARCHY_USER_SHELL").trimmed().isEmpty();
    if ((!paletteOverridden || userShellOverridden)
        && readFontBaseSize(Theme::omarchyUserShellPath(), &result.fontPixelSize)) {
        result.userFontPixelSize = true;
    }

    // Omarchy's global font selector makes fontconfig's monospace alias the
    // source of truth. Resolve the concrete first family so a running app can
    // adopt a newly selected font when the watcher below fires.
    QProcess fontMatch;
    fontMatch.start(QStringLiteral("fc-match"),
                    {QStringLiteral("monospace"), QStringLiteral("-f"),
                     QStringLiteral("%{family}\n")});
    if (fontMatch.waitForFinished(500)
        && fontMatch.exitStatus() == QProcess::NormalExit
        && fontMatch.exitCode() == 0) {
        QString family = QString::fromUtf8(fontMatch.readAllStandardOutput())
                             .section('\n', 0, 0).section(',', 0, 0).trimmed();
        if (!family.isEmpty()) result.fontFamily = family;
    }
    result.valid = true;
    return result;
}

QPalette fixedPalette(bool dark) {
    const QColor background(dark ? QStringLiteral("#2b2b2b") : QStringLiteral("#f0f0f0"));
    const QColor base(dark ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff"));
    const QColor raised(dark ? QStringLiteral("#333333") : QStringLiteral("#f7f7f7"));
    const QColor foreground(dark ? QStringLiteral("#f0f0f0") : QStringLiteral("#1f1f1f"));
    const QColor disabled(dark ? QStringLiteral("#7a7a7a") : QStringLiteral("#a0a0a0"));
    const QColor accent(dark ? QStringLiteral("#2d6ba3") : QStringLiteral("#cde4fa"));
    const QColor onAccent(dark ? QStringLiteral("#ffffff") : QStringLiteral("#1f1f1f"));

    QPalette palette;
    palette.setColor(QPalette::Window, background);
    palette.setColor(QPalette::WindowText, foreground);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, raised);
    palette.setColor(QPalette::Text, foreground);
    palette.setColor(QPalette::Button, raised);
    palette.setColor(QPalette::ButtonText, foreground);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, onAccent);
    palette.setColor(QPalette::ToolTipBase, raised);
    palette.setColor(QPalette::ToolTipText, foreground);
    palette.setColor(QPalette::Link, QColor(QStringLiteral("#4a90d9")));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    return palette;
}

QPalette omarchyQtPalette() {
    const QColor onAccent = readableOn(g_omarchy.accent,
                                       g_omarchy.brightForeground,
                                       g_omarchy.darkerBackground);
    QPalette palette;
    palette.setColor(QPalette::Window, g_omarchy.background);
    palette.setColor(QPalette::WindowText, g_omarchy.foreground);
    palette.setColor(QPalette::Base, g_omarchy.darkBackground);
    palette.setColor(QPalette::AlternateBase, g_omarchy.darkerBackground);
    palette.setColor(QPalette::Text, g_omarchy.foreground);
    palette.setColor(QPalette::Button, g_omarchy.lighterBackground);
    palette.setColor(QPalette::ButtonText, g_omarchy.foreground);
    palette.setColor(QPalette::Highlight, g_omarchy.accent);
    palette.setColor(QPalette::HighlightedText, onAccent);
    palette.setColor(QPalette::ToolTipBase, g_omarchy.lighterBackground);
    palette.setColor(QPalette::ToolTipText, g_omarchy.foreground);
    palette.setColor(QPalette::Link, g_omarchy.blue);
    palette.setColor(QPalette::Light, g_omarchy.lighterBackground);
    palette.setColor(QPalette::Mid, g_omarchy.muted);
    palette.setColor(QPalette::Dark, g_omarchy.darkBackground);
    palette.setColor(QPalette::Shadow, g_omarchy.darkerBackground);
    palette.setColor(QPalette::PlaceholderText, g_omarchy.darkForeground);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, g_omarchy.darkForeground);
    palette.setColor(QPalette::Disabled, QPalette::Text, g_omarchy.darkForeground);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, g_omarchy.darkForeground);
    return palette;
}

int effectiveOmarchyFontPixelSize() {
    if (!g_omarchy.valid) return 12;
    if (g_omarchy.userFontPixelSize) return g_omarchy.fontPixelSize;
    return std::max(1, qRound(g_omarchy.fontPixelSize * g_desktopTextScale));
}

qreal effectiveOmarchyUiScale() {
    return g_omarchy.valid ? effectiveOmarchyFontPixelSize() / 12.0 : 1.0;
}

QString scalePixelMetrics(QString style, qreal scale) {
    if (qFuzzyCompare(scale, 1.0)) return style;

    // Scale typography, padding, controls and radii together, like Omarchy's
    // shell style tokens. One-pixel strokes stay one physical design unit so
    // borders and separators remain crisp rather than becoming heavy.
    const QRegularExpression pixels(QStringLiteral(R"((\d+)px)"));
    QVector<QPair<QPair<int, int>, QString>> replacements;
    QRegularExpressionMatchIterator matches = pixels.globalMatch(style);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        const int value = match.captured(1).toInt();
        if (value <= 1) continue;
        const int scaled = std::max(1, qRound(value * scale));
        replacements.append({{match.capturedStart(0), match.capturedLength(0)},
                             QString::number(scaled) + QStringLiteral("px")});
    }
    for (auto it = replacements.crbegin(); it != replacements.crend(); ++it)
        style.replace(it->first.first, it->first.second, it->second);
    return style;
}

QString omarchyStyleSheet() {
    const QColor onAccent = readableOn(g_omarchy.accent,
                                       g_omarchy.brightForeground,
                                       g_omarchy.darkerBackground);
    QString style = scalePixelMetrics(QString::fromUtf8(kOmarchy),
                                      effectiveOmarchyUiScale());
    const QHash<QString, QColor> colors = {
        {QStringLiteral("@accent@"), g_omarchy.accent},
        {QStringLiteral("@selection@"), g_omarchy.selection},
        {QStringLiteral("@muted@"), g_omarchy.muted},
        {QStringLiteral("@background@"), g_omarchy.background},
        {QStringLiteral("@dark@"), g_omarchy.darkBackground},
        {QStringLiteral("@darker@"), g_omarchy.darkerBackground},
        {QStringLiteral("@lighter@"), g_omarchy.lighterBackground},
        {QStringLiteral("@foreground@"), g_omarchy.foreground},
        {QStringLiteral("@disabled@"), g_omarchy.darkForeground},
        {QStringLiteral("@lightForeground@"), g_omarchy.lightForeground},
        {QStringLiteral("@bright@"), g_omarchy.brightForeground},
        {QStringLiteral("@onAccent@"), onAccent},
        {QStringLiteral("@red@"), g_omarchy.red},
    };
    for (auto it = colors.cbegin(); it != colors.cend(); ++it)
        style.replace(it.key(), colorName(it.value()));
    return style;
}

} // namespace

namespace Theme {

void setScheme(Scheme s) { g_scheme = s; }
Scheme scheme() { return g_scheme; }

bool isDark() {
    switch (g_scheme) {
    case Scheme::Light: return false;
    case Scheme::Dark:  return true;
    case Scheme::Default:
    default:            return g_omarchy.valid ? g_omarchy.dark : osPrefersDark();
    }
}

QString styleSheet() {
    if (usesOmarchyPalette()) return omarchyStyleSheet();
    return QString::fromUtf8(isDark() ? kDark : kLight);
}

QPalette palette() {
    if (usesOmarchyPalette()) return omarchyQtPalette();
    return fixedPalette(isDark());
}

QFont uiFont() {
    // Font family and apparent size are desktop choices, independent of the
    // optional Light/Dark colour override inside paint.software.
    if (g_omarchy.valid) {
        QFont font(g_omarchy.fontFamily);
        font.setPixelSize(effectiveOmarchyFontPixelSize());
        font.setStyleHint(QFont::Monospace);
        font.setFixedPitch(true);
        return font;
    }
    return g_havePlatformFont ? g_platformFont : QApplication::font();
}

QColor color(ColorRole role) {
    if (usesOmarchyPalette()) {
        switch (role) {
        case ColorRole::Background:       return g_omarchy.background;
        case ColorRole::Foreground: return g_omarchy.foreground;
        case ColorRole::SubtleForeground: return g_omarchy.lightForeground;
        case ColorRole::BrightForeground: return g_omarchy.brightForeground;
        case ColorRole::Muted:            return g_omarchy.muted;
        case ColorRole::Accent:           return g_omarchy.accent;
        case ColorRole::Selection:        return g_omarchy.selection;
        case ColorRole::Surface:          return g_omarchy.darkBackground;
        case ColorRole::Raised:           return g_omarchy.lighterBackground;
        case ColorRole::Danger:           return g_omarchy.red;
        case ColorRole::Orange:           return g_omarchy.orange;
        case ColorRole::Success:          return g_omarchy.green;
        case ColorRole::Warning:          return g_omarchy.yellow;
        case ColorRole::Cyan:             return g_omarchy.cyan;
        case ColorRole::Blue:             return g_omarchy.blue;
        case ColorRole::Purple:           return g_omarchy.purple;
        case ColorRole::Secondary:        return g_omarchy.magenta;
        }
    }

    const bool dark = isDark();
    switch (role) {
    case ColorRole::Background:       return QColor(dark ? "#2b2b2b" : "#f0f0f0");
    case ColorRole::Foreground:       return QColor(dark ? "#f0f0f0" : "#1f1f1f");
    case ColorRole::SubtleForeground: return QColor(dark ? "#d0d0d0" : "#42464c");
    case ColorRole::BrightForeground: return QColor(dark ? "#ffffff" : "#111111");
    case ColorRole::Muted:            return QColor(dark ? "#5a5a5a" : "#b4b4b4");
    case ColorRole::Accent:           return QColor("#4a90d9");
    case ColorRole::Selection:        return QColor(dark ? "#2d6ba3" : "#cde4fa");
    case ColorRole::Surface:          return QColor(dark ? "#1e1e1e" : "#ffffff");
    case ColorRole::Raised:           return QColor(dark ? "#333333" : "#f7f7f7");
    case ColorRole::Danger:           return QColor(dark ? "#f7768e" : "#d20f39");
    case ColorRole::Orange:           return QColor(dark ? "#ff9e64" : "#fe640b");
    case ColorRole::Success:          return QColor(dark ? "#9ece6a" : "#40a02b");
    case ColorRole::Warning:          return QColor(dark ? "#e0af68" : "#df8e1d");
    case ColorRole::Cyan:             return QColor(dark ? "#7dcfff" : "#179299");
    case ColorRole::Blue:             return QColor(dark ? "#7aa2f7" : "#1e66f5");
    case ColorRole::Purple:           return QColor(dark ? "#bb9af7" : "#7287fd");
    case ColorRole::Secondary:        return QColor(dark ? "#ad8ee6" : "#8839ef");
    }
    return QColor();
}

bool setDesktopTextScale(qreal scale) {
    if (!std::isfinite(scale) || scale <= 0) scale = 1.0;
    scale = qBound(0.5, scale, 3.0);
    if (qFuzzyCompare(g_desktopTextScale, scale)) return false;
    g_desktopTextScale = scale;
    return true;
}

qreal desktopTextScale() { return g_desktopTextScale; }

qreal uiScale() { return effectiveOmarchyUiScale(); }

int scaledMetric(int pixels) {
    if (pixels <= 1) return std::max(0, pixels);
    return std::max(1, qRound(pixels * uiScale()));
}

QPixmap iconCanvas(int logicalSize) {
    qreal density = 1.0;
    for (QScreen *screen : QGuiApplication::screens())
        density = std::max(density, screen->devicePixelRatio());
    density = qBound(1.0, density, 4.0);

    const int physicalSize = std::max(1, qCeil(logicalSize * density));
    QPixmap pixmap(physicalSize, physicalSize);
    pixmap.setDevicePixelRatio(density);
    pixmap.fill(Qt::transparent);
    return pixmap;
}

QString canvasBackdrop() {
    if (usesOmarchyPalette()) return colorName(g_omarchy.darkerBackground);
    // paint.net surrounds the image with a mid gray in light mode and a much
    // darker gray in dark mode.
    return isDark() ? "#3c3c3c" : "#969696";
}

QString omarchyPalettePath() {
    const QString override = qEnvironmentVariable("PAINTSW_OMARCHY_COLORS").trimmed();
    if (!override.isEmpty()) return QFileInfo(override).absoluteFilePath();

    QString stateHome = qEnvironmentVariable("XDG_STATE_HOME").trimmed();
    if (stateHome.isEmpty())
        stateHome = QDir::home().filePath(QStringLiteral(".local/state"));
    return QDir(stateHome).filePath(QStringLiteral("omarchy/current/theme/colors.toml"));
}

QString omarchyUserShellPath() {
    const QString override =
        qEnvironmentVariable("PAINTSW_OMARCHY_USER_SHELL").trimmed();
    if (!override.isEmpty()) return QFileInfo(override).absoluteFilePath();

    QString configHome = qEnvironmentVariable("XDG_CONFIG_HOME").trimmed();
    if (configHome.isEmpty()) configHome = QDir::home().filePath(QStringLiteral(".config"));
    return QDir(configHome).filePath(QStringLiteral("omarchy/shell.toml"));
}

bool reloadExternalPalette() {
    const OmarchyPalette loaded = readOmarchyPalette(omarchyPalettePath());
    if (samePalette(g_omarchy, loaded)) return false;
    g_omarchy = loaded;
    return true;
}

bool usesOmarchyPalette() {
    return g_scheme == Scheme::Default && g_omarchy.valid;
}

void loadFromSettings() {
    if (!g_havePlatformFont) {
        g_platformFont = QApplication::font();
        g_havePlatformFont = true;
    }
    QSettings s("PaintDali", "PaintDali");
    const QString v = s.value("ui/colorScheme", "default").toString();
    if (v == "light") g_scheme = Scheme::Light;
    else if (v == "dark") g_scheme = Scheme::Dark;
    else g_scheme = Scheme::Default;
    reloadExternalPalette();
}

void saveToSettings() {
    QSettings s("PaintDali", "PaintDali");
    const char *v = (g_scheme == Scheme::Light) ? "light"
                  : (g_scheme == Scheme::Dark)  ? "dark" : "default";
    s.setValue("ui/colorScheme", v);
}

} // namespace Theme
