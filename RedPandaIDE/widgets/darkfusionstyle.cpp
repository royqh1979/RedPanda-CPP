/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <QPainter>
#include <QStyleOption>
#include <QGroupBox>
#include <qdrawutil.h>
#include <QPainterPath>
#include <QCoreApplication>
#include <QPixmapCache>
#include <QApplication>
#include <QSvgRenderer>
#include <QScrollBar>
#include "darkfusionstyle.h"
#include "../settings.h"

#if defined(Q_OS_MACX)
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#endif

#define BEGIN_STYLE_PIXMAPCACHE(a) \
    QRect rect = option->rect; \
    QPixmap internalPixmapCache; \
    QImage imageCache; \
    QPainter *p = painter; \
    QString unique = QStyleHelper::uniqueName((a), option, option->rect.size()); \
    int txType = painter->deviceTransform().type() | painter->worldTransform().type(); \
    bool doPixmapCache = (!option->rect.isEmpty()) \
            && ((txType <= QTransform::TxTranslate) || (painter->deviceTransform().type() == QTransform::TxScale)); \
    if (doPixmapCache && QPixmapCache::find(unique, &internalPixmapCache)) { \
        painter->drawPixmap(option->rect.topLeft(), internalPixmapCache); \
    } else { \
        if (doPixmapCache) { \
            rect.setRect(0, 0, option->rect.width(), option->rect.height()); \
            imageCache = styleCacheImage(option->rect.size()); \
            imageCache.fill(0); \
            p = new QPainter(&imageCache); \
        }



#define END_STYLE_PIXMAPCACHE \
        if (doPixmapCache) { \
            p->end(); \
            delete p; \
            internalPixmapCache = QPixmap::fromImage(imageCache); \
            painter->drawPixmap(option->rect.topLeft(), internalPixmapCache); \
            QPixmapCache::insert(unique, internalPixmapCache); \
        } \
    }


namespace QStyleHelper {


static QString uniqueName(const QString &key, const QStyleOption *option, const QSize &size)
{
    const QStyleOptionComplex *complexOption = qstyleoption_cast<const QStyleOptionComplex *>(option);
    QString tmp = QString("%1%2%3%4%5%6%7")
            .arg(key)
            .arg(option->state)
            .arg(option->direction)
            .arg(complexOption ? uint(complexOption->activeSubControls) : 0u)
            .arg(option->palette.cacheKey())
            .arg(size.width())
            .arg(size.height());

#if QT_CONFIG(spinbox)
    if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
        tmp = tmp + QString("%1%2%3")
                .arg(spinBox->buttonSymbols)
                .arg(spinBox->stepEnabled)
                .arg(QLatin1Char(spinBox->frame ? '1' : '0'));
    }
#endif // QT_CONFIG(spinbox)

    return tmp;
}

static qreal calcDpi() {
    return screenDPI();
}

static qreal calcDpiScaled(qreal value, qreal dpi) {
    return value*dpi/96;
}

}


enum Direction {
    TopDown,
    FromLeft,
    BottomUp,
    FromRight
};

// from windows style
//static const int windowsItemFrame        =  2; // menu item frame width
//static const int windowsItemHMargin      =  3; // menu item hor text margin
//static const int windowsItemVMargin      =  8; // menu item ver text margin
//static const int windowsRightBorder      = 15; // right border on windows

//static const int groupBoxBottomMargin    =  0;  // space below the groupbox
//static const int groupBoxTopMargin       =  3;

DarkFusionStyle::DarkFusionStyle():QProxyStyle("fusion")
{

}
// On mac we want a standard blue color used when the system palette is used
static bool isMacSystemPalette(const QPalette &pal){
    Q_UNUSED(pal);
#if defined(Q_OS_MACX)
    const QPalette *themePalette = QGuiApplicationPrivate::platformTheme()->palette();
    if (themePalette && themePalette->color(QPalette::Normal, QPalette::Highlight) ==
            pal.color(QPalette::Normal, QPalette::Highlight) &&
        themePalette->color(QPalette::Normal, QPalette::HighlightedText) ==
            pal.color(QPalette::Normal, QPalette::HighlightedText))
        return true;
#endif
    return false;
}

// Used for grip handles
static QColor calcDarkShade() {
    return QColor(255, 255, 255, 150);
}
static QColor calcLightShade() {
    return QColor(0, 0, 0, 60);
}

// The default button and handle gradient
static QLinearGradient qt_fusion_gradient(const QRect &rect, const QBrush &baseColor, Direction direction = TopDown)
{
    int x = rect.center().x();
    int y = rect.center().y();
    QLinearGradient gradient;
    switch (direction) {
    case FromLeft:
        gradient = QLinearGradient(rect.left(), y, rect.right(), y);
        break;
    case FromRight:
        gradient = QLinearGradient(rect.right(), y, rect.left(), y);
        break;
    case BottomUp:
        gradient = QLinearGradient(x, rect.bottom(), x, rect.top());
        break;
    case TopDown:
    default:
        gradient = QLinearGradient(x, rect.top(), x, rect.bottom());
        break;
    }
    if (baseColor.gradient())
        gradient.setStops(baseColor.gradient()->stops());
    else {
        QColor gradientStartColor = baseColor.color().lighter(124);
        QColor gradientStopColor = baseColor.color().lighter(102);
        gradient.setColorAt(0, gradientStartColor);
        gradient.setColorAt(1, gradientStopColor);
        //          Uncomment for adding shiny shading
        //            QColor midColor1 = mergedColors(gradientStartColor, gradientStopColor, 55);
        //            QColor midColor2 = mergedColors(gradientStartColor, gradientStopColor, 45);
        //            gradient.setColorAt(0.5, midColor1);
        //            gradient.setColorAt(0.501, midColor2);
    }
    return gradient;
}


static QColor calcHighlight(const QPalette &pal) {
    if (isMacSystemPalette(pal))
        return QColor(60, 140, 230);
    return pal.color(QPalette::Highlight);
}

static QColor calcBackgroundColor(const QPalette &pal, const QWidget* widget)
{
#if QT_CONFIG(scrollarea)
    if (qobject_cast<const QScrollBar *>(widget) && widget->parent() &&
            qobject_cast<const QAbstractScrollArea *>(widget->parent()->parent()))
        return widget->parentWidget()->parentWidget()->palette().color(QPalette::Base);
#else
    Q_UNUSED(widget);
#endif
    return pal.color(QPalette::Base);
}

static QColor calcOutline(const QPalette &pal) {
    if (pal.window().style() == Qt::TexturePattern)
        return QColor(255, 255, 255, 160);
    return pal.window().color().lighter(180);
}

inline QColor calcHighlightedOutline(const QPalette &pal) {
    QColor highlightedOutline = calcHighlight(pal).lighter(125);
    if (highlightedOutline.value() > 160)
        highlightedOutline.setHsl(highlightedOutline.hue(), highlightedOutline.saturation(), 160);
    return highlightedOutline;
}

inline QColor calcButtonColor(const QPalette &pal) {
    QColor buttonColor = pal.button().color();
    int val = qGray(buttonColor.rgb());
    buttonColor = buttonColor.darker(100 + qMax(1, (180 - val)/6));
    buttonColor.setHsv(buttonColor.hue(), buttonColor.saturation() * 0.75, buttonColor.value());
    return buttonColor;
}

static QColor calcTabFrameColor(const QPalette &pal) {
    if (pal.window().style() == Qt::TexturePattern)
        return QColor(255, 255, 255, 8);
    return calcButtonColor(pal).lighter(104);
}

inline QColor calcTopShadow() {
    return QColor(255, 255, 255, 18);
}

inline QColor calcInnerContrastLine() {
    return QColor(0, 0, 0, 30);
}

static QColor mergedColors(const QColor &colorA, const QColor &colorB, int factor = 50)
{
    const int maxFactor = 100;
    QColor tmp = colorA;
    tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
    tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
    tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
    return tmp;
}

inline QImage styleCacheImage(const QSize &size)
{
    const qreal pixelRatio = qApp->devicePixelRatio();
    QImage cacheImage = QImage(size * pixelRatio, QImage::Format_ARGB32_Premultiplied);
    cacheImage.setDevicePixelRatio(pixelRatio);
    return cacheImage;
}

inline QPixmap styleCachePixmap(const QSize &size)
{
    const qreal pixelRatio = qApp->devicePixelRatio();
    QPixmap cachePixmap = QPixmap(size * pixelRatio);
    cachePixmap.setDevicePixelRatio(pixelRatio);
    return cachePixmap;
}

static void dark_fusion_draw_arrow(Qt::ArrowType type, QPainter *painter, const QStyleOption *option, const QRect &rect, const QColor &color)
{
    if (rect.isEmpty())
        return;

    const qreal dpi = QStyleHelper::calcDpi();
    const int arrowWidth = int(QStyleHelper::calcDpiScaled(14, dpi));
    const int arrowHeight = int(QStyleHelper::calcDpiScaled(8, dpi));

    const int arrowMax = qMin(arrowHeight, arrowWidth);
    const int rectMax = qMin(rect.height(), rect.width());
    const int size = qMin(arrowMax, rectMax);

    QPixmap cachePixmap;
    QString cacheKey =
            QString("%1-%2-%3")
            .arg(QStyleHelper::uniqueName(QLatin1String("dark-fusion-arrow"), option, rect.size()))
            .arg(type)
            .arg(color.rgba());
    if (!QPixmapCache::find(cacheKey, &cachePixmap)) {
        cachePixmap = styleCachePixmap(rect.size());
        cachePixmap.fill(Qt::transparent);
        QPainter cachePainter(&cachePixmap);

        QRectF arrowRect;
        arrowRect.setWidth(size);
        arrowRect.setHeight(arrowHeight * size / arrowWidth);
        if (type == Qt::LeftArrow || type == Qt::RightArrow)
            arrowRect = arrowRect.transposed();
        arrowRect.moveTo((rect.width() - arrowRect.width()) / 2.0,
                         (rect.height() - arrowRect.height()) / 2.0);

        QPolygonF triangle;
        triangle.reserve(3);
        switch (type) {
        case Qt::DownArrow:
            triangle << arrowRect.topLeft() << arrowRect.topRight() << QPointF(arrowRect.center().x(), arrowRect.bottom());
            break;
        case Qt::RightArrow:
            triangle << arrowRect.topLeft() << arrowRect.bottomLeft() << QPointF(arrowRect.right(), arrowRect.center().y());
            break;
        case Qt::LeftArrow:
            triangle << arrowRect.topRight() << arrowRect.bottomRight() << QPointF(arrowRect.left(), arrowRect.center().y());
            break;
        default:
            triangle << arrowRect.bottomLeft() << arrowRect.bottomRight() << QPointF(arrowRect.center().x(), arrowRect.top());
            break;
        }

        cachePainter.setPen(Qt::NoPen);
        cachePainter.setBrush(color);
        cachePainter.setRenderHint(QPainter::Antialiasing);
        cachePainter.drawPolygon(triangle);

        QPixmapCache::insert(cacheKey, cachePixmap);
    }

    painter->drawPixmap(rect, cachePixmap);
}



void DarkFusionStyle::drawPrimitive(PrimitiveElement elem, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    Q_ASSERT(option);

    QRect rect = option->rect;
    int state = option->state;

    QColor outline = calcOutline(option->palette);
    QColor highlightedOutline = calcHighlightedOutline(option->palette);

    QColor tabFrameColor = calcTabFrameColor(option->palette);

    switch (elem) {

//#if QT_CONFIG(groupbox)
//    // No frame drawn
//    case PE_FrameGroupBox:
//    {
//        QPixmap pixmap(QLatin1String(":/qt-project.org/styles/commonstyle/images/fusion_groupbox.png"));
//        int topMargin = 0;
//        auto control = qobject_cast<const QGroupBox *>(widget);
//        if (control && !control->isCheckable() && control->title().isEmpty()) {
//            // Shrinking the topMargin if Not checkable AND title is empty
//            topMargin = groupBoxTopMargin;
//        } else {
//            topMargin = qMax(pixelMetric(PM_ExclusiveIndicatorHeight), option->fontMetrics.height()) + groupBoxTopMargin;
//        }
//        QRect frame = option->rect.adjusted(0, topMargin, 0, 0);
//        qDrawBorderPixmap(painter, frame, QMargins(6, 6, 6, 6), pixmap);
//        break;
//    }
//#endif // QT_CONFIG(groupbox)
//    case PE_IndicatorBranch: {
//        if (!(option->state & State_Children))
//            break;
//        if (option->state & State_Open)
//            drawPrimitive(PE_IndicatorArrowDown, option, painter, widget);
//        else {
//            const bool reverse = (option->direction == Qt::RightToLeft);
//            drawPrimitive(reverse ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight, option, painter, widget);
//        }
//        break;
//    }
#if QT_CONFIG(tabbar)
    case PE_FrameTabBarBase:
        if (const QStyleOptionTabBarBase *tbb
                = qstyleoption_cast<const QStyleOptionTabBarBase *>(option)) {
            painter->save();
            painter->setPen(QPen(outline.darker(110)));
            switch (tbb->shape) {
            case QTabBar::RoundedNorth: {
                QRegion region(tbb->rect);
                region -= tbb->selectedTabRect;
                painter->drawLine(tbb->rect.topLeft(), tbb->rect.topRight());
                painter->setClipRegion(region);
                painter->setPen(option->palette.light().color());
                painter->drawLine(tbb->rect.topLeft() + QPoint(0, 1), tbb->rect.topRight() + QPoint(0, 1));
            }
                break;
            case QTabBar::RoundedWest:
                painter->drawLine(tbb->rect.left(), tbb->rect.top(), tbb->rect.left(), tbb->rect.bottom());
                break;
            case QTabBar::RoundedSouth:
                painter->drawLine(tbb->rect.left(), tbb->rect.bottom(),
                                  tbb->rect.right(), tbb->rect.bottom());
                break;
            case QTabBar::RoundedEast:
                painter->drawLine(tbb->rect.topRight(), tbb->rect.bottomRight());
                break;
            case QTabBar::TriangularNorth:
            case QTabBar::TriangularEast:
            case QTabBar::TriangularWest:
            case QTabBar::TriangularSouth:
                painter->restore();
                QCommonStyle::drawPrimitive(elem, option, painter, widget);
                return;
            }
            painter->restore();
        }
        return;
#endif // QT_CONFIG(tabbar)
    case PE_PanelScrollAreaCorner: {
        painter->save();
        QColor alphaOutline = outline;
        alphaOutline.setAlpha(180);
        painter->setPen(alphaOutline);
        painter->setBrush(option->palette.brush(QPalette::Window));
        painter->drawRect(option->rect);
        painter->restore();
    } break;
//    case PE_IndicatorArrowUp:
//    case PE_IndicatorArrowDown:
//    case PE_IndicatorArrowRight:
//    case PE_IndicatorArrowLeft:
//    {
//        if (option->rect.width() <= 1 || option->rect.height() <= 1)
//            break;
//        QColor arrowColor = option->palette.windowText().color();
//        arrowColor.setAlpha(160);
//        Qt::ArrowType arrow = Qt::UpArrow;
//        switch (elem) {
//        case PE_IndicatorArrowDown:
//            arrow = Qt::DownArrow;
//            break;
//        case PE_IndicatorArrowRight:
//            arrow = Qt::RightArrow;
//            break;
//        case PE_IndicatorArrowLeft:
//            arrow = Qt::LeftArrow;
//            break;
//        default:
//            break;
//        }
//        qt_fusion_draw_arrow(arrow, painter, option, option->rect, arrowColor);
//    }
//        break;
//    case PE_IndicatorItemViewItemCheck:
//    {
//        QStyleOptionButton button;
//        button.QStyleOption::operator=(*option);
//        button.state &= ~State_MouseOver;
//        proxy()->drawPrimitive(PE_IndicatorCheckBox, &button, painter, widget);
//    }
//        return;
//    case PE_IndicatorHeaderArrow:
//        if (const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
//            QRect r = header->rect;
//            QColor arrowColor = header->palette.windowText().color();
//            arrowColor.setAlpha(180);
//            QPoint offset = QPoint(0, -2);

//#if defined(Q_OS_LINUX)
//            if (header->sortIndicator & QStyleOptionHeader::SortUp) {
//                qt_fusion_draw_arrow(Qt::UpArrow, painter, option, r.translated(offset), arrowColor);
//            } else if (header->sortIndicator & QStyleOptionHeader::SortDown) {
//                qt_fusion_draw_arrow(Qt::DownArrow, painter, option, r.translated(offset), arrowColor);
//            }
//#else
//            if (header->sortIndicator & QStyleOptionHeader::SortUp) {
//                qt_fusion_draw_arrow(Qt::DownArrow, painter, option, r.translated(offset), arrowColor);
//            } else if (header->sortIndicator & QStyleOptionHeader::SortDown) {
//                qt_fusion_draw_arrow(Qt::UpArrow, painter, option, r.translated(offset), arrowColor);
//            }
//#endif
//        }
//        break;
//    case PE_IndicatorButtonDropDown:
//        proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
//        break;

    case PE_IndicatorToolBarSeparator:
    {
        QRect rect = option->rect;
        const int margin = 6;
        if (option->state & State_Horizontal) {
            const int offset = rect.width()/2;
            painter->setPen(QPen(option->palette.window().color().lighter(110)));
            painter->drawLine(rect.bottomLeft().x() + offset,
                              rect.bottomLeft().y() - margin,
                              rect.topLeft().x() + offset,
                              rect.topLeft().y() + margin);
            painter->setPen(QPen(option->palette.window().color().darker(110)));
            painter->drawLine(rect.bottomLeft().x() + offset + 1,
                              rect.bottomLeft().y() - margin,
                              rect.topLeft().x() + offset + 1,
                              rect.topLeft().y() + margin);
        } else { //Draw vertical separator
            const int offset = rect.height()/2;
            painter->setPen(QPen(option->palette.window().color().lighter(110)));
            painter->drawLine(rect.topLeft().x() + margin ,
                              rect.topLeft().y() + offset,
                              rect.topRight().x() - margin,
                              rect.topRight().y() + offset);
            painter->setPen(QPen(option->palette.window().color().darker(110)));
            painter->drawLine(rect.topLeft().x() + margin ,
                              rect.topLeft().y() + offset + 1,
                              rect.topRight().x() - margin,
                              rect.topRight().y() + offset + 1);
        }
    }
        break;
    case PE_Frame: {
        if (widget && widget->inherits("QComboBoxPrivateContainer")){
            QStyleOption copy = *option;
            copy.state |= State_Raised;
            proxy()->drawPrimitive(PE_PanelMenu, &copy, painter, widget);
            break;
        }
        painter->save();
        QPen thePen(outline.darker(108));
        thePen.setCosmetic(false);
        painter->setPen(thePen);
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->restore(); }
        break;
    case PE_FrameMenu:
        painter->save();
    {
        painter->setPen(QPen(outline));
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        QColor frameLight = option->palette.window().color().darker(160);
        QColor frameShadow = option->palette.window().color().lighter(110);

        //paint beveleffect
        QRect frame = option->rect.adjusted(1, 1, -1, -1);
        painter->setPen(frameLight);
        painter->drawLine(frame.topLeft(), frame.bottomLeft());
        painter->drawLine(frame.topLeft(), frame.topRight());

        painter->setPen(frameShadow);
        painter->drawLine(frame.topRight(), frame.bottomRight());
        painter->drawLine(frame.bottomLeft(), frame.bottomRight());
    }
        painter->restore();
        break;
    case PE_FrameDockWidget:

        painter->save();
    {
        QColor softshadow = option->palette.window().color().lighter(120);

        QRect rect= option->rect;
        painter->setPen(softshadow);
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->setPen(QPen(option->palette.light(), 1));
        painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1), QPoint(rect.left() + 1, rect.bottom() - 1));
        painter->setPen(QPen(option->palette.window().color().lighter(120)));
        painter->drawLine(QPoint(rect.left() + 1, rect.bottom() - 1), QPoint(rect.right() - 2, rect.bottom() - 1));
        painter->drawLine(QPoint(rect.right() - 1, rect.top() + 1), QPoint(rect.right() - 1, rect.bottom() - 1));

    }
        painter->restore();
        break;
//    case PE_PanelButtonTool:
//        painter->save();
//        if ((option->state & State_Enabled || option->state & State_On) || !(option->state & State_AutoRaise)) {
//            if (widget && widget->inherits("QDockWidgetTitleButton")) {
//                if (option->state & State_MouseOver)
//                    proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
//            } else {
//                proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
//            }
//        }
//        painter->restore();
//        break;
//    case PE_IndicatorDockWidgetResizeHandle:
//    {
//        QStyleOption dockWidgetHandle = *option;
//        bool horizontal = option->state & State_Horizontal;
//        dockWidgetHandle.state.setFlag(State_Horizontal, !horizontal);
//        proxy()->drawControl(CE_Splitter, &dockWidgetHandle, painter, widget);
//    }
//        break;
    case PE_FrameWindow:
        painter->save();
    {
        QRect rect= option->rect;
        painter->setPen(QPen(outline.lighter(150)));
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->setPen(QPen(option->palette.light(), 1));
        painter->drawLine(QPoint(rect.left() + 1, rect.top() + 1),
                          QPoint(rect.left() + 1, rect.bottom() - 1));
        painter->setPen(QPen(option->palette.window().color().darker(120)));
        painter->drawLine(QPoint(rect.left() + 1, rect.bottom() - 1),
                          QPoint(rect.right() - 2, rect.bottom() - 1));
        painter->drawLine(QPoint(rect.right() - 1, rect.top() + 1),
                          QPoint(rect.right() - 1, rect.bottom() - 1));
    }
        painter->restore();
        break;
    case PE_FrameLineEdit:
    {
        QRect r = rect;
        bool hasFocus = option->state & State_HasFocus;

        painter->save();

        painter->setRenderHint(QPainter::Antialiasing, true);
        //  ### highdpi painter bug.
        painter->translate(0.5, 0.5);

        // Draw Outline
        painter->setPen( QPen(hasFocus ? highlightedOutline : outline));
        painter->drawRoundedRect(r.adjusted(0, 0, -1, -1), 2, 2);

        if (hasFocus) {
            QColor softHighlight = highlightedOutline;
            softHighlight.setAlpha(40);
            painter->setPen(softHighlight);
            painter->drawRoundedRect(r.adjusted(1, 1, -2, -2), 1.7, 1.7);
        }
        // Draw inner shadow
        painter->setPen(calcTopShadow());
        painter->drawLine(QPoint(r.left() + 2, r.top() + 1), QPoint(r.right() - 2, r.top() + 1));

        painter->restore();

    }
        break;
    case PE_IndicatorCheckBox:
        painter->save();
        if (const QStyleOptionButton *checkbox = qstyleoption_cast<const QStyleOptionButton*>(option)) {
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);
            rect = rect.adjusted(0, 0, -1, -1);

            QColor pressedColor = mergedColors(option->palette.base().color(), option->palette.windowText().color(), 85);
            painter->setBrush(Qt::NoBrush);

            // Gradient fill
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0, (state & State_Sunken) ? pressedColor : option->palette.base().color().lighter(115));
            gradient.setColorAt(0.15, (state & State_Sunken) ? pressedColor : option->palette.base().color());
            gradient.setColorAt(1, (state & State_Sunken) ? pressedColor : option->palette.base().color());

            painter->setBrush((state & State_Sunken) ? QBrush(pressedColor) : gradient);
            painter->setPen(QPen(outline.lighter(110)));

            if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange)
                painter->setPen(QPen(highlightedOutline));
            painter->drawRect(rect);

            QColor checkMarkColor = option->palette.text().color().lighter(120);
            const qreal checkMarkPadding = 1 + rect.width() * 0.13; // at least one pixel padding

            if (checkbox->state & State_NoChange) {
                gradient = QLinearGradient(rect.topLeft(), rect.bottomLeft());
                checkMarkColor.setAlpha(80);
                gradient.setColorAt(0, checkMarkColor);
                checkMarkColor.setAlpha(140);
                gradient.setColorAt(1, checkMarkColor);
                checkMarkColor.setAlpha(180);
                painter->setPen(QPen(checkMarkColor, 1));
                painter->setBrush(gradient);
                painter->drawRect(rect.adjusted(checkMarkPadding, checkMarkPadding, -checkMarkPadding, -checkMarkPadding));

            } else if (checkbox->state & State_On) {

                const qreal dpi = QStyleHelper::calcDpi();

                qreal penWidth = QStyleHelper::calcDpiScaled(1.5, dpi);
                penWidth = qMax<qreal>(penWidth, 0.13 * rect.height());
                penWidth = qMin<qreal>(penWidth, 0.20 * rect.height());
                QPen checkPen = QPen(checkMarkColor, penWidth);
                checkMarkColor.setAlpha(210);
                painter->translate(QStyleHelper::calcDpiScaled(-0.8, dpi), QStyleHelper::calcDpiScaled(0.5, dpi));
                painter->setPen(checkPen);
                painter->setBrush(Qt::NoBrush);

                // Draw checkmark
                QPainterPath path;
                const qreal rectHeight = rect.height(); // assuming height equals width
                path.moveTo(checkMarkPadding + rectHeight * 0.11, rectHeight * 0.47);
                path.lineTo(rectHeight * 0.5, rectHeight - checkMarkPadding);
                path.lineTo(rectHeight - checkMarkPadding, checkMarkPadding);
                painter->drawPath(path.translated(rect.topLeft()));
            }
        }
        painter->restore();
        break;
    case PE_IndicatorRadioButton:
        painter->save();
    {
        QColor pressedColor = mergedColors(option->palette.base().color(), option->palette.windowText().color(), 85);
        painter->setBrush((state & State_Sunken) ? pressedColor : option->palette.base().color());
        painter->setRenderHint(QPainter::Antialiasing, true);
        QPainterPath circle;
        const QPointF circleCenter = rect.center() + QPoint(1, 1);
        const qreal outlineRadius = (rect.width() + (rect.width() + 1) % 2) / 2.0 - 1;
        circle.addEllipse(circleCenter, outlineRadius, outlineRadius);
        painter->setPen(QPen(option->palette.window().color().lighter(150)));
        if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange)
            painter->setPen(QPen(highlightedOutline));
        painter->drawPath(circle);

        if (state & (State_On )) {
            circle = QPainterPath();
            const qreal checkmarkRadius = outlineRadius / 2.32;
            circle.addEllipse(circleCenter, checkmarkRadius, checkmarkRadius);
            QColor checkMarkColor = option->palette.text().color().lighter(120);
            checkMarkColor.setAlpha(200);
            painter->setPen(checkMarkColor);
            checkMarkColor.setAlpha(180);
            painter->setBrush(checkMarkColor);
            painter->drawPath(circle);
        }
    }
        painter->restore();
        break;
    case PE_IndicatorToolBarHandle:
    {
        //draw grips
        if (option->state & State_Horizontal) {
            for (int i = -3 ; i < 2 ; i += 3) {
                for (int j = -8 ; j < 10 ; j += 3) {
                    painter->fillRect(rect.center().x() + i, rect.center().y() + j, 2, 2, calcLightShade());
                    painter->fillRect(rect.center().x() + i, rect.center().y() + j, 1, 1, calcDarkShade());
                }
            }
        } else { //vertical toolbar
            for (int i = -6 ; i < 12 ; i += 3) {
                for (int j = -3 ; j < 2 ; j += 3) {
                    painter->fillRect(rect.center().x() + i, rect.center().y() + j, 2, 2, calcLightShade());
                    painter->fillRect(rect.center().x() + i, rect.center().y() + j, 1, 1, calcDarkShade());
                }
            }
        }
        break;
    }
    case PE_FrameDefaultButton:
        break;
    case PE_FrameFocusRect:
        if (const QStyleOptionFocusRect *fropt = qstyleoption_cast<const QStyleOptionFocusRect *>(option)) {
            //### check for d->alt_down
            if (!(fropt->state & State_KeyboardFocusChange))
                return;
            QRect rect = option->rect;

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);
            QColor fillcolor = highlightedOutline;
            fillcolor.setAlpha(80);
            painter->setPen(fillcolor.lighter(120));
            fillcolor.setAlpha(30);
            QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
            gradient.setColorAt(0, fillcolor.darker(160));
            gradient.setColorAt(1, fillcolor);
            painter->setBrush(gradient);
            painter->drawRoundedRect(option->rect.adjusted(0, 0, -1, -1), 1, 1);
            painter->restore();
        }
        break;
    case PE_PanelButtonCommand:
    {
        bool isDefault = false;
        bool isFlat = false;
        bool isDown = (option->state & State_Sunken) || (option->state & State_On);
        QRect r;

        if (const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton*>(option)) {
            isDefault = (button->features & QStyleOptionButton::DefaultButton) && (button->state & State_Enabled);
            isFlat = (button->features & QStyleOptionButton::Flat);
        }

        if (isFlat && !isDown) {
            if (isDefault) {
                r = option->rect.adjusted(0, 1, 0, -1);
                painter->setPen(QPen(Qt::lightGray));
                const QLine lines[4] = {
                    QLine(QPoint(r.left() + 2, r.top()),
                    QPoint(r.right() - 2, r.top())),
                    QLine(QPoint(r.left(), r.top() + 2),
                    QPoint(r.left(), r.bottom() - 2)),
                    QLine(QPoint(r.right(), r.top() + 2),
                    QPoint(r.right(), r.bottom() - 2)),
                    QLine(QPoint(r.left() + 2, r.bottom()),
                    QPoint(r.right() - 2, r.bottom()))
                };
                painter->drawLines(lines, 4);
                const QPoint points[4] = {
                    QPoint(r.right() - 1, r.bottom() - 1),
                    QPoint(r.right() - 1, r.top() + 1),
                    QPoint(r.left() + 1, r.bottom() - 1),
                    QPoint(r.left() + 1, r.top() + 1)
                };
                painter->drawPoints(points, 4);
            }
            return;
        }


        bool isEnabled = option->state & State_Enabled;
        bool hasFocus = (option->state & State_HasFocus && option->state & State_KeyboardFocusChange);
        QColor buttonColor = calcButtonColor(option->palette);

        QColor darkOutline = outline;
        if (hasFocus | isDefault) {
            darkOutline = highlightedOutline;
        }

        if (isDefault)
            buttonColor = mergedColors(buttonColor, highlightedOutline.lighter(130), 90);

        BEGIN_STYLE_PIXMAPCACHE(QStringLiteral("pushbutton-") + buttonColor.name(QColor::HexArgb))
        r = rect.adjusted(0, 1, -1, 0);

        p->setRenderHint(QPainter::Antialiasing, true);
        p->translate(0.5, -0.5);

        QLinearGradient gradient = qt_fusion_gradient(rect, (isEnabled && option->state & State_MouseOver ) ? buttonColor : buttonColor.darker(104));
        p->setPen(Qt::transparent);
        p->setBrush(isDown ? QBrush(buttonColor.lighter(250)) : gradient);
        p->drawRoundedRect(r, 2.0, 2.0);
        p->setBrush(Qt::NoBrush);

        // Outline
        p->setPen(!isEnabled ? QPen(darkOutline.lighter(115)) : QPen(darkOutline));
        p->drawRoundedRect(r, 2.0, 2.0);

        p->setPen(calcInnerContrastLine());
        p->drawRoundedRect(r.adjusted(1, 1, -1, -1), 2.0, 2.0);

        END_STYLE_PIXMAPCACHE
        }
        break;
    case PE_FrameTabWidget:
        painter->save();
        painter->fillRect(option->rect.adjusted(0, 0, -1, -1), tabFrameColor);
#if QT_CONFIG(tabwidget)
        if (const QStyleOptionTabWidgetFrame *twf = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option)) {
            QColor borderColor = outline.lighter(110);
            QRect rect = option->rect.adjusted(0, 0, -1, -1);

            // Shadow outline
            if (twf->shape != QTabBar::RoundedSouth) {
                rect.adjust(0, 0, 0, -1);
                QColor alphaShadow(Qt::black);
                alphaShadow.setAlpha(15);
                painter->setPen(alphaShadow);
                painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());            painter->setPen(borderColor);
            }

            // outline
            painter->setPen(outline);
            painter->drawRect(rect);

            // Inner frame highlight
            painter->setPen(calcInnerContrastLine());
            painter->drawRect(rect.adjusted(1, 1, -1, -1));

        }
#endif // QT_CONFIG(tabwidget)
        painter->restore();
        break ;

    case PE_FrameStatusBarItem:
        break;
    case PE_IndicatorTabClose:
    {
        QIcon closeIcon = proxy()->standardIcon(SP_DialogCloseButton, option, widget);
        if ((option->state & State_Enabled) && (option->state & State_MouseOver))
            proxy()->drawPrimitive(PE_PanelButtonCommand, option, painter, widget);
        int size = pointToPixel(pSettings->environment().interfaceFontSize());
        QPixmap pixmap = closeIcon.pixmap(QSize(size, size), QIcon::Normal, QIcon::On);
        proxy()->drawItemPixmap(painter, option->rect, Qt::AlignCenter, pixmap);
    }
        break;
    case PE_PanelMenu: {
        painter->save();
        const QBrush menuBackground = option->palette.base().color().darker(108);
        QColor borderColor = option->palette.window().color().lighter(160);
        qDrawPlainRect(painter, option->rect, borderColor, 1, &menuBackground);
        painter->restore();
    }
        break;
    default:
        QProxyStyle::drawPrimitive(elem, option, painter, widget);
        break;
    }
}

QIcon DarkFusionStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option, const QWidget *widget) const
{
    switch (standardIcon) {
    case SP_TitleBarCloseButton:
    case SP_DockWidgetCloseButton:
    case SP_DialogCloseButton: {
        int size = pointToPixel(pSettings->environment().interfaceFontSize());
        QSvgRenderer renderer(QString(":/icons/images/dark-close.svg"));
        QPixmap pixmap(size,size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter,pixmap.rect());
        return QIcon(pixmap);
    }
    default:
        break;
    }

    return QProxyStyle::standardIcon(standardIcon, option, widget);
}

void DarkFusionStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
#if QT_CONFIG(slider)
   QColor buttonColor = calcButtonColor(option->palette);
   QColor gradientStopColor = buttonColor;
   QColor gradientStartColor = buttonColor.lighter(118);
   QColor outline = calcOutline(option->palette);

#endif
    switch(control) {
#if QT_CONFIG(slider)
    case CC_ScrollBar:
        painter->save();
        if (const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            bool wasActive = false;
            qreal expandScale = 1.0;
            qreal expandOffset = -1.0;
            QObject *styleObject = option->styleObject;
            if (styleObject && proxy()->styleHint(SH_ScrollBar_Transient, option, widget)) {

                int oldPos = styleObject->property("_q_stylepos").toInt();
                int oldMin = styleObject->property("_q_stylemin").toInt();
                int oldMax = styleObject->property("_q_stylemax").toInt();
                QRect oldRect = styleObject->property("_q_stylerect").toRect();
                QStyle::State oldState = static_cast<QStyle::State>(qvariant_cast<QStyle::State::Int>(styleObject->property("_q_stylestate")));
                uint oldActiveControls = styleObject->property("_q_stylecontrols").toUInt();

                // a scrollbar is transient when the scrollbar itself and
                // its sibling are both inactive (ie. not pressed/hovered/moved)
                bool transient = !option->activeSubControls && !(option->state & State_On);

                if (!transient ||
                        oldPos != scrollBar->sliderPosition ||
                        oldMin != scrollBar->minimum ||
                        oldMax != scrollBar->maximum ||
                        oldRect != scrollBar->rect ||
                        oldState != scrollBar->state ||
                        oldActiveControls != scrollBar->activeSubControls) {

                    styleObject->setProperty("_q_stylepos", scrollBar->sliderPosition);
                    styleObject->setProperty("_q_stylemin", scrollBar->minimum);
                    styleObject->setProperty("_q_stylemax", scrollBar->maximum);
                    styleObject->setProperty("_q_stylerect", scrollBar->rect);
                    styleObject->setProperty("_q_stylestate", static_cast<QStyle::State::Int>(scrollBar->state));
                    styleObject->setProperty("_q_stylecontrols", static_cast<uint>(scrollBar->activeSubControls));

                }

            }

            bool transient = proxy()->styleHint(SH_ScrollBar_Transient, option, widget);
            bool horizontal = scrollBar->orientation == Qt::Horizontal;
            bool sunken = scrollBar->state & State_Sunken;

            QRect scrollBarSubLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSubLine, widget);
            QRect scrollBarAddLine = proxy()->subControlRect(control, scrollBar, SC_ScrollBarAddLine, widget);
            QRect scrollBarSlider = proxy()->subControlRect(control, scrollBar, SC_ScrollBarSlider, widget);
            QRect scrollBarGroove = proxy()->subControlRect(control, scrollBar, SC_ScrollBarGroove, widget);

            QRect rect = option->rect;
            QColor alphaOutline = outline;
            alphaOutline.setAlpha(180);

            QColor arrowColor = option->palette.windowText().color();
            arrowColor.setAlpha(160);

            const QColor bgColor =  calcBackgroundColor(option->palette, widget);
            const bool isDarkBg = bgColor.red() < 128 && bgColor.green() < 128 && bgColor.blue() < 128;

            if (transient) {
                if (horizontal) {
                    rect.setY(rect.y() + 4.5 - expandOffset);
                    scrollBarSlider.setY(scrollBarSlider.y() + 4.5 - expandOffset);
                    scrollBarGroove.setY(scrollBarGroove.y() + 4.5 - expandOffset);

                    rect.setHeight(rect.height() * expandScale);
                    scrollBarGroove.setHeight(scrollBarGroove.height() * expandScale);
                } else {
                    rect.setX(rect.x() + 4.5 - expandOffset);
                    scrollBarSlider.setX(scrollBarSlider.x() + 4.5 - expandOffset);
                    scrollBarGroove.setX(scrollBarGroove.x() + 4.5 - expandOffset);

                    rect.setWidth(rect.width() * expandScale);
                    scrollBarGroove.setWidth(scrollBarGroove.width() * expandScale);
                }
            }

            // Paint groove
            if ((!transient || scrollBar->activeSubControls || wasActive) && scrollBar->subControls & SC_ScrollBarGroove) {
                QLinearGradient gradient(rect.center().x(), rect.top(),
                                         rect.center().x(), rect.bottom());
                if (!horizontal)
                    gradient = QLinearGradient(rect.left(), rect.center().y(),
                                               rect.right(), rect.center().y());
                if (!transient || !isDarkBg) {
                    gradient.setColorAt(0, buttonColor.darker(107));
                    gradient.setColorAt(0.1, buttonColor.darker(105));
                    gradient.setColorAt(0.9, buttonColor.darker(105));
                    gradient.setColorAt(1, buttonColor.darker(107));
                } else {
                    gradient.setColorAt(0, bgColor.lighter(157));
                    gradient.setColorAt(0.1, bgColor.lighter(155));
                    gradient.setColorAt(0.9, bgColor.lighter(155));
                    gradient.setColorAt(1, bgColor.lighter(157));
                }

                painter->save();
                if (transient)
                    painter->setOpacity(0.8);
                painter->fillRect(rect, gradient);
                painter->setPen(Qt::NoPen);
                if (transient)
                    painter->setOpacity(0.4);
                painter->setPen(alphaOutline);
                if (horizontal)
                    painter->drawLine(rect.topLeft(), rect.topRight());
                else
                    painter->drawLine(rect.topLeft(), rect.bottomLeft());

                QColor subtleEdge = alphaOutline;
                subtleEdge.setAlpha(40);
                painter->setPen(subtleEdge);
                painter->setBrush(Qt::NoBrush);
                painter->setClipRect(scrollBarGroove.adjusted(1, 0, -1, -3));
                painter->drawRect(scrollBarGroove.adjusted(1, 0, -1, -1));
                painter->restore();
            }

            QRect pixmapRect = scrollBarSlider;
            QLinearGradient gradient(pixmapRect.center().x(), pixmapRect.top(),
                                     pixmapRect.center().x(), pixmapRect.bottom());
            if (!horizontal)
                gradient = QLinearGradient(pixmapRect.left(), pixmapRect.center().y(),
                                           pixmapRect.right(), pixmapRect.center().y());

            QLinearGradient highlightedGradient = gradient;

            QColor midColor2 = mergedColors(gradientStartColor, gradientStopColor, 40);
            gradient.setColorAt(0, calcButtonColor(option->palette).lighter(108));
            gradient.setColorAt(1, calcButtonColor(option->palette));

            highlightedGradient.setColorAt(0, gradientStartColor.darker(102));
            highlightedGradient.setColorAt(1, gradientStopColor.lighter(102));

            // Paint slider
            if (scrollBar->subControls & SC_ScrollBarSlider) {
                if (transient) {
                    QRect rect = scrollBarSlider.adjusted(horizontal ? 1 : 2, horizontal ? 2 : 1, -1, -1);
                    painter->setPen(Qt::NoPen);
                    painter->setBrush(isDarkBg ? calcLightShade() : calcDarkShade());
                    int r = qMin(rect.width(), rect.height()) / 2;

                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->drawRoundedRect(rect, r, r);
                    painter->restore();
                } else {
                    QLinearGradient gradient;
                    QLinearGradient highlightedGradient;
                    gradient.setColorAt(0, buttonColor.lighter(200));
                    gradient.setColorAt(1, buttonColor.lighter(200));

                    highlightedGradient.setColorAt(0, buttonColor.lighter(200));
                    highlightedGradient.setColorAt(1, buttonColor.lighter(250));
                    QRect pixmapRect = scrollBarSlider;
                    painter->setPen(QPen(alphaOutline));
                    if (option->state & State_Sunken && scrollBar->activeSubControls & SC_ScrollBarSlider)
                        painter->setBrush(gradient);
                    else if (option->state & State_MouseOver && scrollBar->activeSubControls & SC_ScrollBarSlider)
                        painter->setBrush(highlightedGradient);
                    else
                        painter->setBrush(gradient);

                    painter->drawRect(pixmapRect.adjusted(horizontal ? -1 : 0, horizontal ? 0 : -1, horizontal ? 0 : 1, horizontal ? 1 : 0));

                    painter->setPen(calcInnerContrastLine());
                    painter->drawRect(scrollBarSlider.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, -1, -1));

                    // Outer shadow
                    //                  painter->setPen(subtleEdge);
                    //                  if (horizontal) {
                    ////                    painter->drawLine(scrollBarSlider.topLeft() + QPoint(-2, 0), scrollBarSlider.bottomLeft() + QPoint(2, 0));
                    ////                    painter->drawLine(scrollBarSlider.topRight() + QPoint(-2, 0), scrollBarSlider.bottomRight() + QPoint(2, 0));
                    //                  } else {
                    ////                    painter->drawLine(pixmapRect.topLeft() + QPoint(0, -2), pixmapRect.bottomLeft() + QPoint(0, -2));
                    ////                    painter->drawLine(pixmapRect.topRight() + QPoint(0, 2), pixmapRect.bottomRight() + QPoint(0, 2));
                    //                  }
                }
            }

            // The SubLine (up/left) buttons
            if (!transient && scrollBar->subControls & SC_ScrollBarSubLine) {
                if ((scrollBar->activeSubControls & SC_ScrollBarSubLine) && sunken)
                    painter->setBrush(gradientStopColor);
                else if ((scrollBar->activeSubControls & SC_ScrollBarSubLine))
                    painter->setBrush(highlightedGradient);
                else
                    painter->setBrush(gradient);

                painter->setPen(Qt::NoPen);
                painter->drawRect(scrollBarSubLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, 0, 0));
                painter->setPen(QPen(alphaOutline));
                if (option->state & State_Horizontal) {
                    if (option->direction == Qt::RightToLeft) {
                        pixmapRect.setLeft(scrollBarSubLine.left());
                        painter->drawLine(pixmapRect.topLeft(), pixmapRect.bottomLeft());
                    } else {
                        pixmapRect.setRight(scrollBarSubLine.right());
                        painter->drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
                    }
                } else {
                    pixmapRect.setBottom(scrollBarSubLine.bottom());
                    painter->drawLine(pixmapRect.bottomLeft(), pixmapRect.bottomRight());
                }

                QRect upRect = scrollBarSubLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, horizontal ? -2 : -1, horizontal ? -1 : -2);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(calcInnerContrastLine());
                painter->drawRect(upRect);

                // Arrows
                Qt::ArrowType arrowType = Qt::UpArrow;
                if (option->state & State_Horizontal)
                    arrowType = option->direction == Qt::LeftToRight ? Qt::LeftArrow : Qt::RightArrow;
                dark_fusion_draw_arrow(arrowType, painter, option, upRect, arrowColor);
            }

            // The AddLine (down/right) button
            if (!transient && scrollBar->subControls & SC_ScrollBarAddLine) {
                if ((scrollBar->activeSubControls & SC_ScrollBarAddLine) && sunken)
                    painter->setBrush(gradientStopColor);
                else if ((scrollBar->activeSubControls & SC_ScrollBarAddLine))
                    painter->setBrush(midColor2);
                else
                    painter->setBrush(gradient);

                painter->setPen(Qt::NoPen);
                painter->drawRect(scrollBarAddLine.adjusted(horizontal ? 0 : 1, horizontal ? 1 : 0, 0, 0));
                painter->setPen(QPen(alphaOutline, 1));
                if (option->state & State_Horizontal) {
                    if (option->direction == Qt::LeftToRight) {
                        pixmapRect.setLeft(scrollBarAddLine.left());
                        painter->drawLine(pixmapRect.topLeft(), pixmapRect.bottomLeft());
                    } else {
                        pixmapRect.setRight(scrollBarAddLine.right());
                        painter->drawLine(pixmapRect.topRight(), pixmapRect.bottomRight());
                    }
                } else {
                    pixmapRect.setTop(scrollBarAddLine.top());
                    painter->drawLine(pixmapRect.topLeft(), pixmapRect.topRight());
                }

                QRect downRect = scrollBarAddLine.adjusted(1, 1, -1, -1);
                painter->setPen(calcInnerContrastLine());
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(downRect);

                Qt::ArrowType arrowType = Qt::DownArrow;
                if (option->state & State_Horizontal)
                    arrowType = option->direction == Qt::LeftToRight ? Qt::RightArrow : Qt::LeftArrow;
                dark_fusion_draw_arrow(arrowType, painter, option, downRect, arrowColor);
            }

        }
        painter->restore();
        break;;
#endif // QT_CONFIG(slider)
    default:
        QProxyStyle::drawComplexControl(control,option,painter,widget);
    }

}

int DarkFusionStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch ( metric ) {
    case QStyle::PM_SmallIconSize:
        return pointToPixel(pSettings->environment().interfaceFontSize());
    case QStyle::PM_TabCloseIndicatorHeight:
    case QStyle::PM_TabCloseIndicatorWidth:
        return 1.2*pointToPixel(pSettings->environment().interfaceFontSize());
    default:
        return QProxyStyle::pixelMetric( metric, option, widget );
    }
}

void DarkFusionStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                               const QWidget *widget) const
{
    QRect rect = option->rect;
    QColor outline = calcOutline(option->palette);
    //QColor highlightedOutline = calcHighlightedOutline(option->palette);
    QColor shadow = calcDarkShade();


    switch (element) {
    case CE_MenuItem:
        // Draws one item in a popup menu.
        if (const QStyleOptionMenuItem *menuItem = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            //QColor highlightOutline = highlightedOutline;
            //QColor highlight = option->palette.highlight().color();
            if (menuItem->menuItemType == QStyleOptionMenuItem::Separator) {
                painter->save();
                int w = 0;
                qreal dpi = QStyleHelper::calcDpi();
                const int margin = int(QStyleHelper::calcDpiScaled(5, dpi));
                if (!menuItem->text.isEmpty()) {
                    painter->setFont(menuItem->font);
                    proxy()->drawItemText(painter, menuItem->rect.adjusted(margin, 0, -margin, 0), Qt::AlignLeft | Qt::AlignVCenter,
                                          menuItem->palette, menuItem->state & State_Enabled, menuItem->text,
                                          QPalette::Text);
                    w = menuItem->fontMetrics.horizontalAdvance(menuItem->text) + margin;
                }
                painter->setPen(shadow.darker(150));
                bool reverse = menuItem->direction == Qt::RightToLeft;
                painter->drawLine(menuItem->rect.left() + margin + (reverse ? 0 : w), menuItem->rect.center().y(),
                                  menuItem->rect.right() - margin - (reverse ? w : 0), menuItem->rect.center().y());
                painter->restore();
                break;
            }
        }
        QProxyStyle::drawControl(element, option, painter, widget);
        break;
    case CE_TabBarTabShape:
        painter->save();
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {

            bool rtlHorTabs = (tab->direction == Qt::RightToLeft
                               && (tab->shape == QTabBar::RoundedNorth
                                   || tab->shape == QTabBar::RoundedSouth));
            bool selected = tab->state & State_Selected;
            bool lastTab = ((!rtlHorTabs && tab->position == QStyleOptionTab::End)
                            || (rtlHorTabs
                                && tab->position == QStyleOptionTab::Beginning));
            bool onlyOne = tab->position == QStyleOptionTab::OnlyOneTab;
            int tabOverlap = pixelMetric(PM_TabBarTabOverlap, option, widget);
            rect = option->rect.adjusted(0, 0, (onlyOne || lastTab) ? 0 : tabOverlap, 0);

            QRect r2(rect);
            int x1 = r2.left();
            int x2 = r2.right();
            int y1 = r2.top();
            int y2 = r2.bottom();

            painter->setPen(calcInnerContrastLine());

            QTransform rotMatrix;
            bool flip = false;
            painter->setPen(shadow);

            switch (tab->shape) {
            case QTabBar::RoundedNorth:
                break;
            case QTabBar::RoundedSouth:
                rotMatrix.rotate(180);
                rotMatrix.translate(0, -rect.height() + 1);
                rotMatrix.scale(-1, 1);
                painter->setTransform(rotMatrix, true);
                break;
            case QTabBar::RoundedWest:
                rotMatrix.rotate(180 + 90);
                rotMatrix.scale(-1, 1);
                flip = true;
                painter->setTransform(rotMatrix, true);
                break;
            case QTabBar::RoundedEast:
                rotMatrix.rotate(90);
                rotMatrix.translate(0, - rect.width() + 1);
                flip = true;
                painter->setTransform(rotMatrix, true);
                break;
            default:
                painter->restore();
                QCommonStyle::drawControl(element, tab, painter, widget);
                return;
            }

            if (flip) {
                QRect tmp = rect;
                rect = QRect(tmp.y(), tmp.x(), tmp.height(), tmp.width());
                int temp = x1;
                x1 = y1;
                y1 = temp;
                temp = x2;
                x2 = y2;
                y2 = temp;
            }

            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->translate(0.5, 0.5);

            QColor tabFrameColor = tab->features & QStyleOptionTab::HasFrame ?
                        calcTabFrameColor(option->palette) :
                        option->palette.window().color();

            QLinearGradient fillGradient(rect.topLeft(), rect.bottomLeft());
            QLinearGradient outlineGradient(rect.topLeft(), rect.bottomLeft());
            QPen outlinePen = outline.lighter(110);
            if (selected) {
                fillGradient.setColorAt(0, tabFrameColor.lighter(250));
                //                QColor highlight = option->palette.highlight().color();
                //                if (option->state & State_HasFocus && option->state & State_KeyboardFocusChange) {
                //                    fillGradient.setColorAt(0, highlight.lighter(130));
                //                    outlineGradient.setColorAt(0, highlight.darker(130));
                //                    fillGradient.setColorAt(0.14, highlight);
                //                    outlineGradient.setColorAt(0.14, highlight.darker(130));
                //                    fillGradient.setColorAt(0.1401, tabFrameColor);
                //                    outlineGradient.setColorAt(0.1401, highlight.darker(130));
                //                }
                fillGradient.setColorAt(0.85, tabFrameColor.lighter(150));
                fillGradient.setColorAt(1, tabFrameColor);
                outlineGradient.setColorAt(1, outline);
                outlinePen = QPen(outlineGradient, 1);
            } else {
                fillGradient.setColorAt(0, tabFrameColor.darker(108));
                fillGradient.setColorAt(0.85, tabFrameColor.darker(108));
                fillGradient.setColorAt(1, tabFrameColor.darker(116));
            }

            QRect drawRect = rect.adjusted(0, selected ? 0 : 2, 0, 3);
            painter->setPen(outlinePen);
            painter->save();
            painter->setClipRect(rect.adjusted(-1, -1, 1, selected ? -2 : -3));
            painter->setBrush(fillGradient);
            painter->drawRoundedRect(drawRect.adjusted(0, 0, -1, -1), 2.0, 2.0);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(calcInnerContrastLine());
            painter->drawRoundedRect(drawRect.adjusted(1, 1, -2, -1), 2.0, 2.0);
            painter->restore();

            if (selected) {
                painter->fillRect(rect.left() + 1, rect.bottom() - 1, rect.width() - 2, rect.bottom() - 1, tabFrameColor);
                painter->fillRect(QRect(rect.bottomRight() + QPoint(-2, -1), QSize(1, 1)), calcInnerContrastLine());
                painter->fillRect(QRect(rect.bottomLeft() + QPoint(0, -1), QSize(1, 1)), calcInnerContrastLine());
                painter->fillRect(QRect(rect.bottomRight() + QPoint(-1, -1), QSize(1, 1)), calcInnerContrastLine());
            }
        }
        painter->restore();
        break;
    default:
        QProxyStyle::drawControl(element, option, painter, widget);
        break;
    }
}
