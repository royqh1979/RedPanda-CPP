#include "iconsmanager.h"

IconsManager* pIconsManager;

IconsManager::IconsManager(QObject *parent) : QObject(parent)
{
    mSyntaxError = std::make_shared<QPixmap>(":/icons/images/editor/syntaxerror.png");
    mSyntaxWarning = std::make_shared<QPixmap>(":/icons/images/editor/syntaxwarning.png");
    mBreakpoint = std::make_shared<QPixmap>(":/icons/images/editor/breakpoint.png");
    mCurrentLine = std::make_shared<QPixmap>(":/icons/images/editor/currentline.png");
}

PIcon IconsManager::syntaxError() const
{
    return mSyntaxError;
}

PIcon IconsManager::syntaxWarning() const
{
    return mSyntaxWarning;
}

PIcon IconsManager::breakpoint() const
{
    return mBreakpoint;
}

PIcon IconsManager::currentLine() const
{
    return mCurrentLine;
}
