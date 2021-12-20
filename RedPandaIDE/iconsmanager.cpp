#include "iconsmanager.h"

#include <QPainter>
#include <QSvgRenderer>

IconsManager* pIconsManager;

IconsManager::IconsManager(QObject *parent) : QObject(parent)
{
    updateEditorGuttorIcons(24);
    mFolder = std::make_shared<QPixmap>(":/icons/images/newlook24/090-explorer.png");

}

void IconsManager::updateEditorGuttorIcons(int size)
{
    mBreakpoint = createSVGIcon(":/icons/images/editor/breakpoint.svg",size,size);
    mSyntaxError = createSVGIcon(":/icons/images/editor/syntaxerror.svg",size,size);
    mSyntaxWarning = createSVGIcon(":/icons/images/editor/syntaxwarning.svg",size,size);
    mActiveBreakpoint = createSVGIcon(":/icons/images/editor/currentline.svg",size,size);
    mBookmark = createSVGIcon(":/icons/images/editor/bookmark.svg",size,size);
}

const PIcon &IconsManager::syntaxError() const
{
    return mSyntaxError;
}

const PIcon &IconsManager::syntaxWarning() const
{
    return mSyntaxWarning;
}

const PIcon &IconsManager::breakpoint() const
{
    return mBreakpoint;
}

const PIcon &IconsManager::activeBreakpoint() const
{
    return mActiveBreakpoint;
}

const PIcon &IconsManager::bookmark() const
{
    return mBookmark;
}

const PIcon &IconsManager::folder() const
{
    return mFolder;
}

PIcon IconsManager::createSVGIcon(const QString &filename, int width, int height)
{
    QSvgRenderer renderer(filename);
    PIcon icon = std::make_shared<QPixmap>(width,height);
    icon->fill(Qt::transparent);
    QPainter painter(icon.get());
    renderer.render(&painter,icon->rect());
    return icon;
}
