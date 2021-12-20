#include "iconsmanager.h"

#include <QPainter>
#include <QSvgRenderer>

IconsManager* pIconsManager;

IconsManager::IconsManager(QObject *parent) : QObject(parent)
{
    mDefaultIcon = PIcon();
    //updateEditorGuttorIcons(24);
    mFolder = std::make_shared<QPixmap>(":/icons/images/newlook24/090-explorer.png");

}

void IconsManager::updateEditorGuttorIcons(const QString& iconSet,int size)
{
    QString iconFolder = QString(":/icons/images/%1/").arg(iconSet);
    mIcons.insert(GUTTER_BREAKPOINT, createSVGIcon(iconFolder+"breakpoint.svg",size,size));
    mIcons.insert(GUTTER_SYNTAX_ERROR, createSVGIcon(iconFolder+"syntaxerror.svg",size,size));
    mIcons.insert(GUTTER_SYNTAX_WARNING,createSVGIcon(iconFolder+"syntaxwarning.svg",size,size));
    mIcons.insert(GUTTER_ACTIVEBREAKPOINT,createSVGIcon(iconFolder+"currentline.svg",size,size));
    mIcons.insert(GUTTER_BOOKMARK, createSVGIcon(iconFolder+"bookmark.svg",size,size));
}

void IconsManager::updateParserIcons(const QString &iconSet, int size)
{
    QString iconFolder = QString(":/icons/images/%1/").arg(iconSet);
    mIcons.insert(PARSER_TYPE, createSVGIcon(iconFolder+"type.svg",size,size));
    mIcons.insert(PARSER_CLASS, createSVGIcon(iconFolder+"class.svg",size,size));
    mIcons.insert(PARSER_NAMESPACE, createSVGIcon(iconFolder+"namespace.svg",size,size));
    mIcons.insert(PARSER_DEFINE, createSVGIcon(iconFolder+"define.svg",size,size));
    mIcons.insert(PARSER_ENUM, createSVGIcon(iconFolder+"enum.svg",size,size));;
    mIcons.insert(PARSER_GLOBAL_METHOD, createSVGIcon(iconFolder+"global_method.svg",size,size));
    mIcons.insert(PARSER_INHERITED_PROTECTED_METHOD, createSVGIcon(iconFolder+"method_inherited_protected.svg",size,size));
    mIcons.insert(PARSER_INHERITED_METHOD, createSVGIcon(iconFolder+"method_inherited.svg",size,size));
    mIcons.insert(PARSER_PROTECTED_METHOD, createSVGIcon(iconFolder+"method_protected.svg",size,size));
    mIcons.insert(PARSER_PUBLIC_METHOD, createSVGIcon(iconFolder+"method_public.svg",size,size));
    mIcons.insert(PARSER_PRIVATE_METHOD, createSVGIcon(iconFolder+"method_private.svg",size,size));
    mIcons.insert(PARSER_GLOBAL_VAR, createSVGIcon(iconFolder+"global.svg",size,size));
    mIcons.insert(PARSER_INHERITED_PROTECTD_VAR, createSVGIcon(iconFolder+"var_inherited_protected.svg",size,size));
    mIcons.insert(PARSER_INHERITED_VAR, createSVGIcon(iconFolder+"var_inherited.svg",size,size));
    mIcons.insert(PARSER_PROTECTED_VAR, createSVGIcon(iconFolder+"var_protected.svg",size,size));
    mIcons.insert(PARSER_PUBLIC_VAR, createSVGIcon(iconFolder+"var_public.svg",size,size));
    mIcons.insert(PARSER_PRIVATE_VAR, createSVGIcon(iconFolder+"var_private.svg",size,size));

}

IconsManager::PIcon IconsManager::getIcon(IconName iconName) const
{
    return mIcons.value(iconName, mDefaultIcon);
}

const IconsManager::PIcon &IconsManager::folder() const
{
    return mFolder;
}

IconsManager::PIcon IconsManager::createSVGIcon(const QString &filename, int width, int height)
{
    QSvgRenderer renderer(filename);
    PIcon icon = std::make_shared<QPixmap>(width,height);
    icon->fill(Qt::transparent);
    QPainter painter(icon.get());
    renderer.render(&painter,icon->rect());
    return icon;
}
