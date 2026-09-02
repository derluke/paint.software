#include "toolicons.h"
#include "theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFont>
#include <cmath>

namespace {

static const int TS = 20; // Tool icon size
static const int TBS = 16; // Toolbar icon size

QPixmap mkpm(int size) {
    return Theme::iconCanvas(size);
}

QColor ink()       { return Theme::color(Theme::ColorRole::Foreground); }
QColor mutedInk()  { return Theme::color(Theme::ColorRole::Muted); }
QColor accent()    { return Theme::color(Theme::ColorRole::Accent); }
QColor surface()   { return Theme::color(Theme::ColorRole::Surface); }
QColor raised()    { return Theme::color(Theme::ColorRole::Raised); }
QColor danger()    { return Theme::color(Theme::ColorRole::Danger); }
QColor success()   { return Theme::color(Theme::ColorRole::Success); }
QColor warning()   { return Theme::color(Theme::ColorRole::Warning); }
QColor cyan()       { return Theme::color(Theme::ColorRole::Cyan); }
QColor blue()       { return Theme::color(Theme::ColorRole::Blue); }
QColor secondary() { return Theme::color(Theme::ColorRole::Secondary); }

QPen iconPen(const QColor &color, qreal width = 1.4) {
    QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    return pen;
}

void aa(QPainter &p) {
    p.setRenderHint(QPainter::Antialiasing, true);
}

// ==================== TOOL ICONS (20x20) ====================

// ---- Colourful, paint.net-style tool icons -------------------------------
// Their silhouettes remain familiar, but the blues, greens, ambers and violets
// come from the active semantic theme so the full palette changes coherently.

// Small helper: the dashed "marching ants" outline used by the selection tools.
void marchingAnts(QPainter &p) {
    QPen pen(mutedInk(), 1.2, Qt::DashLine);
    QVector<qreal> d; d << 2.5 << 1.8;
    pen.setDashPattern(d);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
}

QPixmap drawRectSelect() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    p.setPen(Qt::NoPen);
    QColor plate = accent(); plate.setAlpha(190);
    p.setBrush(plate);
    p.drawRect(4, 4, 12, 12);
    marchingAnts(p);
    p.drawRect(4, 4, 12, 12);
    p.end();
    return pm;
}

QPixmap drawEllipseSelect() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    p.setPen(Qt::NoPen);
    QColor plate = accent(); plate.setAlpha(190);
    p.setBrush(plate);
    p.drawEllipse(3, 3, 14, 14);
    marchingAnts(p);
    p.drawEllipse(3, 3, 14, 14);
    p.end();
    return pm;
}

QPixmap drawMagicWand() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Slate shaft running bottom-left -> top-right, with a lighter tip.
    QPen shaft(mutedInk(), 3.0);
    shaft.setCapStyle(Qt::RoundCap);
    p.setPen(shaft);
    p.drawLine(3, 17, 11, 9);
    QPen tip(ink(), 3.0);
    tip.setCapStyle(Qt::RoundCap);
    p.setPen(tip);
    p.drawLine(11, 9, 13, 7);
    // Golden sparkles around the tip.
    auto sparkle = [&](qreal cx, qreal cy, qreal r, const QColor &c, qreal w) {
        QPen sp(c, w); sp.setCapStyle(Qt::RoundCap);
        p.setPen(sp);
        p.drawLine(QPointF(cx, cy - r), QPointF(cx, cy + r));
        p.drawLine(QPointF(cx - r, cy), QPointF(cx + r, cy));
    };
    sparkle(15.5, 4.0, 3.2, warning(), 1.6);
    sparkle(9.5, 3.0, 1.9, warning().lighter(125), 1.2);
    sparkle(18.0, 9.5, 1.7, warning().lighter(125), 1.2);
    p.end();
    return pm;
}

// Move Selected Pixels: a FILLED blue arrow + a small 4-way move cross.
QPixmap drawMove() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPainterPath a;
    a.moveTo(2, 1); a.lineTo(2, 12.5); a.lineTo(5.0, 9.8);
    a.lineTo(7.0, 14.2); a.lineTo(8.9, 13.3); a.lineTo(7.0, 9.2);
    a.lineTo(11.0, 8.8); a.closeSubpath();
    p.setPen(QPen(accent().darker(145), 1.0));
    p.setBrush(accent());
    p.drawPath(a);
    // Compact move cross, bottom-right.
    const qreal cx = 14.5, cy = 14.5, r = 5.0;
    QPen cp(mutedInk(), 1.3); cp.setCapStyle(Qt::RoundCap);
    p.setPen(cp);
    p.drawLine(QPointF(cx, cy - r), QPointF(cx, cy + r));
    p.drawLine(QPointF(cx - r, cy), QPointF(cx + r, cy));
    p.setPen(Qt::NoPen);
    p.setBrush(mutedInk());
    const qreal h = 2.0;
    QPointF up[] = {QPointF(cx, cy - r - 0.8), QPointF(cx - h, cy - r + 1.6), QPointF(cx + h, cy - r + 1.6)};
    QPointF dn[] = {QPointF(cx, cy + r + 0.8), QPointF(cx - h, cy + r - 1.6), QPointF(cx + h, cy + r - 1.6)};
    QPointF lf[] = {QPointF(cx - r - 0.8, cy), QPointF(cx - r + 1.6, cy - h), QPointF(cx - r + 1.6, cy + h)};
    QPointF rt[] = {QPointF(cx + r + 0.8, cy), QPointF(cx + r - 1.6, cy - h), QPointF(cx + r - 1.6, cy + h)};
    p.drawPolygon(up, 3); p.drawPolygon(dn, 3); p.drawPolygon(lf, 3); p.drawPolygon(rt, 3);
    p.end();
    return pm;
}

// paint.net's paintbrush: blue handle, steel ferrule, blue bristle point.
QPixmap drawBrush() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QLinearGradient hg(18, 1, 8, 11);
    hg.setColorAt(0.0, accent().lighter(130));
    hg.setColorAt(1.0, accent().darker(115));
    p.setPen(QPen(accent().darker(155), 0.9));
    p.setBrush(hg);
    QPainterPath h;
    h.moveTo(17.4, 1.2); h.lineTo(19.0, 2.8); h.lineTo(10.2, 11.6); h.lineTo(8.4, 9.8);
    h.closeSubpath();
    p.drawPath(h);
    // Ferrule
    p.setPen(QPen(mutedInk(), 0.9));
    p.setBrush(ink());
    QPainterPath f;
    f.moveTo(10.2, 11.6); f.lineTo(8.4, 9.8); f.lineTo(6.2, 12.0); f.lineTo(8.0, 13.8);
    f.closeSubpath();
    p.drawPath(f);
    // Bristles
    p.setPen(Qt::NoPen);
    p.setBrush(accent().darker(130));
    QPainterPath b;
    b.moveTo(6.2, 12.0); b.lineTo(8.0, 13.8); b.lineTo(1.8, 18.6);
    b.closeSubpath();
    p.drawPath(b);
    p.end();
    return pm;
}

// paint.net's pencil: warm amber body, wood collar, graphite point.
QPixmap drawPencil() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QLinearGradient bg(16, 2, 7, 12);
    bg.setColorAt(0.0, warning().lighter(125));
    bg.setColorAt(1.0, warning());
    p.setPen(QPen(warning().darker(155), 0.9));
    p.setBrush(bg);
    QPainterPath body;
    body.moveTo(16.2, 1.4); body.lineTo(18.2, 3.4); body.lineTo(7.6, 13.6); body.lineTo(5.6, 11.6);
    body.closeSubpath();
    p.drawPath(body);
    // Wood collar
    p.setBrush(warning().lighter(145));
    QPainterPath collar;
    collar.moveTo(7.6, 13.6); collar.lineTo(5.6, 11.6); collar.lineTo(4.2, 13.0); collar.lineTo(6.2, 15.0);
    collar.closeSubpath();
    p.drawPath(collar);
    // Graphite tip
    p.setPen(Qt::NoPen);
    p.setBrush(ink());
    QPainterPath tip;
    tip.moveTo(6.2, 15.0); tip.lineTo(4.2, 13.0); tip.lineTo(1.8, 18.4); tip.closeSubpath();
    p.drawPath(tip);
    p.end();
    return pm;
}

// paint.net's eraser: a violet/magenta angled block with a lit top face.
QPixmap drawEraser() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    p.setPen(QPen(secondary().darker(150), 1.0));
    // Top face (light magenta)
    p.setBrush(secondary().lighter(125));
    QPainterPath top;
    top.moveTo(12.8, 2.4); top.lineTo(18.0, 7.6); top.lineTo(10.4, 13.4); top.lineTo(5.2, 8.2);
    top.closeSubpath();
    p.drawPath(top);
    // Front face (deeper violet) -> gives the 3D block feel
    p.setBrush(secondary().darker(120));
    QPainterPath front;
    front.moveTo(5.2, 8.2); front.lineTo(10.4, 13.4); front.lineTo(10.4, 17.2); front.lineTo(5.2, 12.0);
    front.closeSubpath();
    p.drawPath(front);
    // Side face (mid tone)
    p.setBrush(secondary());
    QPainterPath side;
    side.moveTo(10.4, 13.4); side.lineTo(18.0, 7.6); side.lineTo(18.0, 11.0); side.lineTo(10.4, 17.2);
    side.closeSubpath();
    p.drawPath(side);
    p.end();
    return pm;
}

// paint.net's paint bucket: a blue bucket tipped to the left, pouring paint.
QPixmap drawFill() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);

    p.save();
    p.translate(10.0, 9.5);
    p.rotate(-38);                 // tip the bucket like paint.net's
    p.translate(-10.0, -9.5);

    // Handle behind the body
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(mutedInk(), 1.2));
    p.drawArc(QRectF(6.0, 1.6, 8.0, 7.0), 15 * 16, 150 * 16);

    // Tapered body with a blue sheen
    QLinearGradient bg(5.5, 0, 14.5, 0);
    bg.setColorAt(0.0, accent().lighter(130));
    bg.setColorAt(1.0, accent().darker(120));
    p.setPen(QPen(accent().darker(155), 1.0));
    p.setBrush(bg);
    QPainterPath body;
    body.moveTo(5.2, 6.2); body.lineTo(14.8, 6.2);
    body.lineTo(12.9, 16.0); body.lineTo(7.1, 16.0);
    body.closeSubpath();
    p.drawPath(body);

    // Rim (lighter ellipse on top)
    p.setBrush(accent().lighter(145));
    p.drawEllipse(QRectF(5.2, 4.1, 9.6, 4.2));
    p.restore();

    // Paint pouring out to the bottom-right
    p.setPen(Qt::NoPen);
    p.setBrush(accent());
    QPainterPath drip;
    drip.moveTo(12.6, 11.4);
    drip.quadTo(17.6, 14.0, 15.8, 18.4);
    drip.quadTo(13.4, 19.6, 12.2, 15.6);
    drip.closeSubpath();
    p.drawPath(drip);
    p.end();
    return pm;
}

// paint.net's colour picker: a blue-bulbed eyedropper.
QPixmap drawColorPicker() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Bulb
    p.setPen(QPen(accent().darker(155), 0.9));
    p.setBrush(accent());
    p.drawEllipse(QRectF(10.8, 1.0, 7.8, 7.8));
    // Barrel
    p.setPen(QPen(mutedInk(), 0.9));
    p.setBrush(ink());
    QPainterPath shaft;
    shaft.moveTo(12.4, 7.4); shaft.lineTo(14.8, 9.8); shaft.lineTo(7.4, 15.6); shaft.lineTo(5.2, 13.4);
    shaft.closeSubpath();
    p.drawPath(shaft);
    // Tip
    p.setPen(Qt::NoPen);
    p.setBrush(mutedInk());
    QPainterPath tip;
    tip.moveTo(7.4, 15.6); tip.lineTo(5.2, 13.4); tip.lineTo(1.8, 18.6); tip.closeSubpath();
    p.drawPath(tip);
    p.end();
    return pm;
}

// paint.net's recolor: a blue disc swept by a red "swap colour" arrow.
QPixmap drawRecolor() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Blue disc
    p.setPen(QPen(accent().darker(155), 0.9));
    p.setBrush(accent());
    p.drawEllipse(QRectF(2.6, 5.4, 11.2, 11.2));
    // Red sweep
    QPen ap(danger(), 2.1);
    ap.setCapStyle(Qt::RoundCap);
    p.setPen(ap);
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(4.6, 2.4, 13.0, 13.0), 15 * 16, 205 * 16);
    // Arrow head
    p.setPen(Qt::NoPen);
    p.setBrush(danger());
    QPointF ah[] = { QPointF(18.6, 7.6), QPointF(13.6, 6.9), QPointF(16.3, 11.4) };
    p.drawPolygon(ah, 3);
    p.end();
    return pm;
}

// paint.net's clone stamp: dark knob + stem over a wide amber base.
QPixmap drawCloneStamp() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Knob
    p.setPen(QPen(mutedInk(), 0.9));
    p.setBrush(ink());
    p.drawEllipse(QRectF(7.0, 1.0, 6.0, 5.2));
    // Stem
    p.setBrush(mutedInk());
    p.drawRect(QRectF(8.6, 4.8, 2.8, 5.4));
    // Amber base
    p.setPen(QPen(warning().darker(155), 0.9));
    QLinearGradient bg(4, 10, 16, 15);
    bg.setColorAt(0.0, warning().lighter(125));
    bg.setColorAt(1.0, warning());
    p.setBrush(bg);
    QPainterPath base;
    base.moveTo(5.0, 10.2); base.lineTo(15.0, 10.2);
    base.lineTo(16.6, 14.2); base.lineTo(3.4, 14.2);
    base.closeSubpath();
    p.drawPath(base);
    // Foot
    p.setBrush(warning().darker(125));
    p.drawRect(QRectF(3.2, 14.6, 13.6, 2.6));
    p.end();
    return pm;
}

// paint.net's text tool: a single serif "T". Drawn in a mid slate tone so it reads
// on a light AND a dark palette (paint.net can use black — its palette is light).
QPixmap drawText() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QFont font;
    font.setPixelSize(18);
    font.setBold(true);
    font.setFamily("Serif");
    p.setFont(font);
    p.setPen(ink());
    p.drawText(QRect(0, 0, TS, TS), Qt::AlignCenter, "T");
    p.end();
    return pm;
}

// paint.net's line/curve: an S-curve with its two control nodes.
QPixmap drawLine() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPen cp(ink(), 1.9);
    cp.setCapStyle(Qt::RoundCap);
    p.setPen(cp);
    p.setBrush(Qt::NoBrush);
    QPainterPath c;
    c.moveTo(3.4, 16.4);
    c.cubicTo(7.0, 6.0, 13.0, 18.0, 16.8, 4.0);
    p.drawPath(c);
    // Control nodes
    p.setPen(QPen(accent().darker(155), 0.9));
    p.setBrush(accent());
    p.drawEllipse(QPointF(3.4, 16.4), 2.3, 2.3);
    p.drawEllipse(QPointF(16.8, 4.0), 2.3, 2.3);
    p.end();
    return pm;
}

// paint.net's shapes: a blue square, a green triangle and a purple circle overlapping.
QPixmap drawShape() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Blue square (back)
    p.setPen(QPen(accent().darker(150), 0.9));
    p.setBrush(accent());
    p.drawRect(QRectF(1.6, 2.8, 9.4, 9.4));
    // Green triangle (right)
    p.setPen(QPen(success().darker(150), 0.9));
    p.setBrush(success());
    QPointF tri[] = { QPointF(14.6, 6.2), QPointF(19.0, 16.8), QPointF(10.2, 16.8) };
    p.drawPolygon(tri, 3);
    // Purple circle (front)
    p.setPen(QPen(secondary().darker(150), 0.9));
    p.setBrush(secondary());
    p.drawEllipse(QRectF(5.8, 8.2, 8.8, 8.8));
    p.end();
    return pm;
}

// paint.net's gradient icon: a violet -> blue ramp in a rounded square.
QPixmap drawGradient() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QLinearGradient grad(3, 3, 17, 17);
    grad.setColorAt(0.0, secondary());
    grad.setColorAt(1.0, accent());
    p.setPen(QPen(mutedInk(), 1.1));
    p.setBrush(grad);
    p.drawRoundedRect(QRectF(3, 3, 14, 14), 1.6, 1.6);
    p.end();
    return pm;
}

// paint.net's lasso: a rope loop with a tail — NOT a dashed circle (which read
// almost identically to the ellipse-select icon right below it).
QPixmap drawLasso() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPen rope(accent(), 1.7);
    rope.setCapStyle(Qt::RoundCap);
    p.setPen(rope);
    QColor lassoFill = accent(); lassoFill.setAlpha(150);
    p.setBrush(lassoFill);
    p.drawEllipse(QRectF(3.2, 2.0, 12.4, 9.8));
    // Tail curling away from the loop
    p.setBrush(Qt::NoBrush);
    QPainterPath tail;
    tail.moveTo(9.0, 11.8);
    tail.cubicTo(9.4, 15.0, 13.8, 15.4, 12.2, 18.6);
    p.drawPath(tail);
    p.end();
    return pm;
}

// Move Selection: same silhouette as Move Selected Pixels but with a HOLLOW
// arrow — that outline/filled pair is exactly how paint.net tells them apart.
QPixmap drawMoveSelection() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    QPainterPath a;
    a.moveTo(2, 1); a.lineTo(2, 12.5); a.lineTo(5.0, 9.8);
    a.lineTo(7.0, 14.2); a.lineTo(8.9, 13.3); a.lineTo(7.0, 9.2);
    a.lineTo(11.0, 8.8); a.closeSubpath();
    QPen ap(accent(), 1.3);
    ap.setJoinStyle(Qt::RoundJoin);
    p.setPen(ap);
    p.setBrush(Qt::NoBrush);          // hollow
    p.drawPath(a);
    // Same compact move cross as the Move tool.
    const qreal cx = 14.5, cy = 14.5, r = 5.0;
    QPen cp(mutedInk(), 1.3); cp.setCapStyle(Qt::RoundCap);
    p.setPen(cp);
    p.drawLine(QPointF(cx, cy - r), QPointF(cx, cy + r));
    p.drawLine(QPointF(cx - r, cy), QPointF(cx + r, cy));
    p.setPen(Qt::NoPen);
    p.setBrush(mutedInk());
    const qreal h = 2.0;
    QPointF up[] = {QPointF(cx, cy - r - 0.8), QPointF(cx - h, cy - r + 1.6), QPointF(cx + h, cy - r + 1.6)};
    QPointF dn[] = {QPointF(cx, cy + r + 0.8), QPointF(cx - h, cy + r - 1.6), QPointF(cx + h, cy + r - 1.6)};
    QPointF lf[] = {QPointF(cx - r - 0.8, cy), QPointF(cx - r + 1.6, cy - h), QPointF(cx - r + 1.6, cy + h)};
    QPointF rt[] = {QPointF(cx + r + 0.8, cy), QPointF(cx + r - 1.6, cy - h), QPointF(cx + r - 1.6, cy + h)};
    p.drawPolygon(up, 3); p.drawPolygon(dn, 3); p.drawPolygon(lf, 3); p.drawPolygon(rt, 3);
    p.end();
    return pm;
}

// paint.net's zoom: a magnifier with a glassy blue lens.
QPixmap drawZoom() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Handle (drawn first, behind the lens)
    QPen hp(mutedInk(), 2.7);
    hp.setCapStyle(Qt::RoundCap);
    p.setPen(hp);
    p.drawLine(QPointF(12.4, 12.4), QPointF(17.6, 17.6));
    // Lens
    QRadialGradient lg(7.2, 6.6, 7.0);
    QColor lensLight = accent().lighter(155); lensLight.setAlpha(210);
    QColor lensDark = accent(); lensDark.setAlpha(190);
    lg.setColorAt(0.0, lensLight);
    lg.setColorAt(1.0, lensDark);
    p.setPen(QPen(accent().darker(145), 1.6));
    p.setBrush(lg);
    p.drawEllipse(QRectF(2.2, 2.2, 11.6, 11.6));
    // Plus
    QPen pp(accent().darker(145), 1.5);
    pp.setCapStyle(Qt::RoundCap);
    p.setPen(pp);
    p.drawLine(QPointF(8.0, 4.9), QPointF(8.0, 11.1));
    p.drawLine(QPointF(4.9, 8.0), QPointF(11.1, 8.0));
    p.end();
    return pm;
}

QPixmap drawPan() {
    QPixmap pm = mkpm(TS);
    QPainter p(&pm);
    aa(p);
    // Clean open "grab" hand, in paint.net's warm skin tone. The outline is a warm
    // brown (not neutral ink) so the icon reads on a light AND a dark palette
    // while remaining part of the active semantic palette.
    const QColor handInk = warning().darker(145);
    const QColor skin = warning().lighter(145);
    p.setPen(QPen(handInk, 1.1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(skin);

    QPainterPath hand;
    // Palm + wrist as one rounded body.
    hand.addRoundedRect(QRectF(5.5, 8.5, 9.0, 9.5), 3.0, 3.0);
    p.drawPath(hand);

    // Four fingers (rounded bars) rising from the palm.
    auto finger = [&](double x, double topY) {
        p.drawRoundedRect(QRectF(x, topY, 2.0, 8.0 - (topY - 4.0) * 0.0), 1.0, 1.0);
    };
    p.setBrush(skin);
    p.drawRoundedRect(QRectF(6.2, 3.2, 2.0, 7.0), 1.0, 1.0);
    p.drawRoundedRect(QRectF(8.6, 2.4, 2.0, 8.0), 1.0, 1.0);
    p.drawRoundedRect(QRectF(11.0, 3.0, 2.0, 7.4), 1.0, 1.0);
    p.drawRoundedRect(QRectF(13.2, 4.6, 2.0, 6.0), 1.0, 1.0);
    // Thumb on the left.
    p.drawRoundedRect(QRectF(3.2, 9.0, 3.4, 2.1), 1.0, 1.0);
    (void)finger;
    p.end();
    return pm;
}

// ==================== TOOLBAR ICONS (16x16) ====================

QPixmap drawNewDoc() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(ink()));
    p.setBrush(surface());
    QPainterPath page;
    page.moveTo(2.5, 1.5); page.lineTo(9.5, 1.5); page.lineTo(13.5, 5.5);
    page.lineTo(13.5, 14.5); page.lineTo(2.5, 14.5); page.closeSubpath();
    p.drawPath(page);
    p.setBrush(raised());
    QPainterPath fold;
    fold.moveTo(9.5, 1.5); fold.lineTo(9.5, 5.5); fold.lineTo(13.5, 5.5);
    p.drawPath(fold);
    p.setPen(iconPen(accent(), 1.8));
    p.drawLine(QPointF(7.0, 10.8), QPointF(11.8, 10.8));
    p.drawLine(QPointF(9.4, 8.4), QPointF(9.4, 13.2));
    p.end();
    return pm;
}

QPixmap drawOpenDoc() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(ink()));
    p.setBrush(surface());
    QPainterPath front;
    front.moveTo(1.5, 4.5); front.lineTo(1.5, 13.5); front.lineTo(14.5, 13.5);
    front.lineTo(14.5, 5.5); front.lineTo(8.0, 5.5); front.lineTo(6.0, 3.0);
    front.lineTo(2.5, 3.0); front.quadTo(1.5, 3.0, 1.5, 4.5);
    p.drawPath(front);
    p.setPen(iconPen(accent(), 1.7));
    p.drawLine(QPointF(4.0, 9.4), QPointF(12.0, 9.4));
    p.end();
    return pm;
}

QPixmap drawSaveDoc() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(ink()));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(2.0, 1.5, 12.0, 13.0), 1.5, 1.5);
    p.setPen(iconPen(accent(), 1.7));
    p.drawLine(QPointF(8.0, 3.0), QPointF(8.0, 10.0));
    p.drawLine(QPointF(5.5, 7.6), QPointF(8.0, 10.1));
    p.drawLine(QPointF(10.5, 7.6), QPointF(8.0, 10.1));
    p.setPen(iconPen(ink()));
    p.drawLine(QPointF(4.5, 12.5), QPointF(11.5, 12.5));
    p.end();
    return pm;
}

QPixmap drawUndo() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(accent(), 1.8));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRect(3, 4, 10, 10), 45*16, 225*16);
    // Arrow head
    p.setBrush(accent());
    p.setPen(Qt::NoPen);
    QPointF arr[] = {QPointF(3, 4), QPointF(7, 3), QPointF(5, 7)};
    p.drawPolygon(arr, 3);
    p.end();
    return pm;
}

QPixmap drawRedo() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(accent(), 1.8));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRect(3, 4, 10, 10), -45*16, -225*16);
    // Arrow head
    p.setBrush(accent());
    p.setPen(Qt::NoPen);
    QPointF arr[] = {QPointF(13, 4), QPointF(9, 3), QPointF(11, 7)};
    p.drawPolygon(arr, 3);
    p.end();
    return pm;
}

QPixmap drawCut() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(ink(), 1.3));
    p.setBrush(Qt::NoBrush);
    // Left blade
    p.drawEllipse(1, 10, 5, 5);
    p.drawLine(4, 11, 10, 3);
    // Right blade
    p.drawEllipse(10, 10, 5, 5);
    p.drawLine(12, 11, 6, 3);
    // Pivot
    p.setBrush(ink());
    p.drawEllipse(QPointF(8, 7), 1.2, 1.2);
    p.end();
    return pm;
}

QPixmap drawCopy() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setBrush(surface());
    p.setPen(iconPen(mutedInk(), 1.2));
    p.drawRoundedRect(QRectF(4.5, 1.5, 10.0, 10.0), 1.0, 1.0);
    p.setPen(iconPen(ink(), 1.3));
    p.drawRoundedRect(QRectF(1.5, 4.5, 10.0, 10.0), 1.0, 1.0);
    p.setPen(iconPen(accent(), 1.2));
    p.drawLine(QPointF(4.0, 8.0), QPointF(9.0, 8.0));
    p.drawLine(QPointF(4.0, 10.5), QPointF(8.0, 10.5));
    p.end();
    return pm;
}

QPixmap drawPaste() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(ink(), 1.3));
    p.setBrush(surface());
    p.drawRoundedRect(QRectF(2.0, 2.5, 12.0, 12.0), 1.5, 1.5);
    p.setBrush(raised());
    p.drawRoundedRect(QRectF(5.0, 1.0, 6.0, 3.5), 1.2, 1.2);
    p.setPen(iconPen(accent(), 1.2));
    p.drawLine(QPointF(5.0, 7.5), QPointF(11.0, 7.5));
    p.drawLine(QPointF(5.0, 10.0), QPointF(10.0, 10.0));
    p.drawLine(QPointF(5.0, 12.5), QPointF(8.5, 12.5));
    p.end();
    return pm;
}

} // anonymous namespace

namespace ToolIcons {

// Tool icons retain paint.net's colourful silhouettes; application chrome below
// uses Omarchy's semantic foreground/accent colours.
QIcon forTool(ToolType type) {
    switch (type) {
    case ToolType::RectSelection:   return QIcon(drawRectSelect());
    case ToolType::EllipseSelection:return QIcon(drawEllipseSelect());
    case ToolType::LassoSelection:  return QIcon(drawLasso());
    case ToolType::MagicWand:       return QIcon(drawMagicWand());
    case ToolType::Move:            return QIcon(drawMove());
    case ToolType::MoveSelection:   return QIcon(drawMoveSelection());
    case ToolType::Zoom:            return QIcon(drawZoom());
    case ToolType::Pan:             return QIcon(drawPan());
    case ToolType::Fill:            return QIcon(drawFill());
    case ToolType::Gradient:        return QIcon(drawGradient());
    case ToolType::Brush:           return QIcon(drawBrush());
    case ToolType::Eraser:          return QIcon(drawEraser());
    case ToolType::Pencil:          return QIcon(drawPencil());
    case ToolType::ColorPicker:     return QIcon(drawColorPicker());
    case ToolType::CloneStamp:      return QIcon(drawCloneStamp());
    case ToolType::Recolor:         return QIcon(drawRecolor());
    case ToolType::Text:            return QIcon(drawText());
    case ToolType::Line:            return QIcon(drawLine());
    case ToolType::Shape:           return QIcon(drawShape());
    default: return QIcon();
    }
}

QIcon newDoc() { return QIcon(drawNewDoc()); }
QIcon openDoc() { return QIcon(drawOpenDoc()); }
QIcon saveDoc() { return QIcon(drawSaveDoc()); }
QIcon undoAction() { return QIcon(drawUndo()); }
QIcon redoAction() { return QIcon(drawRedo()); }
QIcon cutAction() { return QIcon(drawCut()); }
QIcon copyAction() { return QIcon(drawCopy()); }
QIcon pasteAction() { return QIcon(drawPaste()); }

// ==================== TOOLBAR / MENUBAR EXTRAS (16x16) ====================

QIcon printAction() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(ink(), 1.3));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(2.0, 5.5, 12.0, 6.5), 1.3, 1.3);
    p.drawRect(QRectF(4.0, 1.5, 8.0, 5.0));
    p.setBrush(surface());
    p.drawRect(QRectF(4.0, 9.0, 8.0, 5.5));
    p.setPen(iconPen(accent(), 1.5));
    p.drawPoint(QPointF(11.5, 7.8));
    p.end();
    return QIcon(pm);
}

QIcon cropAction() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(ink(), 1.5));
    p.drawLine(QPointF(4.0, 1.5), QPointF(4.0, 11.8));
    p.drawLine(QPointF(1.5, 11.5), QPointF(11.8, 11.5));
    p.setPen(iconPen(accent(), 1.5));
    p.drawLine(QPointF(11.5, 4.2), QPointF(11.5, 14.5));
    p.drawLine(QPointF(4.2, 4.5), QPointF(14.5, 4.5));
    p.end();
    return QIcon(pm);
}

QIcon deselectAction() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    QPen dashed = iconPen(mutedInk(), 1.1);
    dashed.setStyle(Qt::DashLine);
    p.setPen(dashed);
    p.setBrush(Qt::NoBrush);
    p.drawRect(2, 2, 11, 11);
    p.setPen(iconPen(danger(), 1.7));
    p.drawLine(5, 5, 11, 11);
    p.drawLine(11, 5, 5, 11);
    p.end();
    return QIcon(pm);
}

QIcon pixelGridAction() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    p.setPen(iconPen(mutedInk(), 1.0));
    for (int i = 1; i <= 13; i += 4) {
        p.drawLine(i, 1, i, 14);
        p.drawLine(1, i, 14, i);
    }
    p.end();
    return QIcon(pm);
}

QIcon rulersAction() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(ink(), 1.2));
    p.setBrush(surface());
    p.drawRoundedRect(QRectF(1.5, 5.0, 13.0, 6.0), 1.0, 1.0);
    p.setPen(iconPen(accent(), 1.0));
    for (int x = 3; x < 15; x += 3) p.drawLine(x, 5, x, 8);
    p.end();
    return QIcon(pm);
}

// ---- Utility-window icons (menu bar, right side) ----

QIcon toolsWindow() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(accent(), 1.2));
    p.setBrush(accent());
    p.drawRect(2, 11, 12, 2);
    p.drawRect(7, 3, 2, 8);
    p.drawRect(5, 2, 6, 2);
    p.end();
    return QIcon(pm);
}

QIcon historyWindow() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(accent(), 1.4));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(2, 2, 12, 12);
    p.drawLine(8, 4, 8, 8);
    p.drawLine(8, 8, 11, 9);
    p.end();
    return QIcon(pm);
}

QIcon layersWindow() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(ink(), 1.1));
    p.setBrush(raised());
    p.drawRect(3, 3, 9, 6);
    p.setBrush(accent());
    p.drawRect(5, 7, 9, 6);
    p.end();
    return QIcon(pm);
}

QIcon colorsWindow() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    QLinearGradient g(2, 2, 14, 14);
    g.setColorAt(0.0, danger());
    g.setColorAt(0.2, warning());
    g.setColorAt(0.4, success());
    g.setColorAt(0.6, cyan());
    g.setColorAt(0.8, blue());
    g.setColorAt(1.0, secondary());
    p.setPen(iconPen(ink(), 1.0));
    p.setBrush(g);
    p.drawEllipse(2, 2, 12, 12);
    p.end();
    return QIcon(pm);
}

QIcon settings() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(ink(), 1.3));
    p.setBrush(raised());
    // simple gear: circle + teeth
    p.drawEllipse(4, 4, 8, 8);
    p.setPen(iconPen(ink(), 1.6));
    for (int a = 0; a < 360; a += 45) {
        double r = a * M_PI / 180.0;
        p.drawLine(QPointF(8 + 5.0 * cos(r), 8 + 5.0 * sin(r)),
                   QPointF(8 + 7.0 * cos(r), 8 + 7.0 * sin(r)));
    }
    p.setBrush(surface());
    p.setPen(iconPen(accent(), 1.1));
    p.drawEllipse(6, 6, 4, 4);
    p.end();
    return QIcon(pm);
}

QIcon help() {
    QPixmap pm = mkpm(TBS);
    QPainter p(&pm);
    aa(p);
    p.setPen(iconPen(accent(), 1.4));
    p.setBrush(surface());
    p.drawEllipse(1, 1, 14, 14);
    QFont f = Theme::uiFont();
    f.setPixelSize(10);
    f.setBold(true);
    p.setFont(f);
    p.setPen(accent());
    p.drawText(QRect(1, 1, 14, 14), Qt::AlignCenter, "?");
    p.end();
    return QIcon(pm);
}

} // namespace ToolIcons
