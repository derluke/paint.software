#pragma once

#include <QColor>
#include <QFont>
#include <QPalette>
#include <QPixmap>
#include <QString>

// paint.net's "Color Scheme" setting: Default follows Omarchy when its semantic
// palette is available, then falls back to the OS. Light and Dark force a scheme.
namespace Theme {

enum class Scheme { Default, Light, Dark };
enum class ColorRole {
    Background,
    Foreground,
    SubtleForeground,
    BrightForeground,
    Muted,
    Accent,
    Selection,
    Surface,
    Raised,
    Danger,
    Orange,
    Success,
    Warning,
    Cyan,
    Blue,
    Purple,
    Secondary,
};

void setScheme(Scheme s);
Scheme scheme();

// True when the effective scheme (after resolving Default) is dark.
bool isDark();

// The full application stylesheet for the effective scheme.
QString styleSheet();

// Installs the resolved font, native palette, and stylesheet application-wide.
// Call once before constructing widgets so controls that derive an explicit
// bold/italic font inherit the Omarchy family rather than Qt's startup default.
void applyToApplication();

// Palette roles used by native dialogs and widgets that do not honour every
// stylesheet rule.
QPalette palette();

// Typography and semantic colours shared by painted icons and widgets that
// cannot be styled through QSS alone.
QFont uiFont();
QColor color(ColorRole role);

// Omarchy's desktop text-size control is a scale rooted at 12px. A user-level
// shell.toml override wins when present; otherwise the portal/GTK scale is
// applied to the active theme's base size.
bool setDesktopTextScale(qreal scale);
qreal desktopTextScale();
qreal uiScale();
int scaledMetric(int pixels);

// Create a transparent logical-size canvas at the highest connected screen
// density. Programmatically painted icons remain crisp on HiDPI displays while
// their drawing code can continue to use logical coordinates.
QPixmap iconCanvas(int logicalSize);

// Canvas backdrop colour (around the image) for the effective scheme.
QString canvasBackdrop();

// Omarchy publishes the active theme here. An override is supported for tests
// and custom setups via PAINTSW_OMARCHY_COLORS.
QString omarchyPalettePath();
QString omarchyUserShellPath();
bool reloadExternalPalette();
bool usesOmarchyPalette();

void loadFromSettings();
void saveToSettings();

} // namespace Theme
