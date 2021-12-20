#include "iconsmanager.h"

#include <QPainter>
#include <QSvgRenderer>

IconsManager* pIconsManager;

IconsManager::IconsManager(QObject *parent) : QObject(parent)
{
    updateIcons(24);

}

void IconsManager::updateIcons(int size)
{
    QSvgRenderer renderer(QString(":/icons/images/editor/breakpoint.svg"));
    mBreakpoint = std::make_shared<QPixmap>(size,size);
    mBreakpoint->fill(Qt::transparent);
    QPainter painter(mBreakpoint.get());
    renderer.render(&painter,mBreakpoint->rect());

    mSyntaxError = std::make_shared<QPixmap>(":/icons/images/editor/syntaxerror.png");
    mSyntaxWarning = std::make_shared<QPixmap>(":/icons/images/editor/syntaxwarning.png");
    mActiveBreakpoint = std::make_shared<QPixmap>(":/icons/images/editor/currentline.png");
    mBookmark = std::make_shared<QPixmap>(":/icons/images/editor/bookmark.png");
    mFolder = std::make_shared<QPixmap>(":/icons/images/newlook24/090-explorer.png");
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
