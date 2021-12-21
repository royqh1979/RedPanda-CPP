#include "iconsmanager.h"

#include <QPainter>
#include <QSvgRenderer>
#include <QDebug>

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

void IconsManager::updateActionIcons(const QString iconSet, int size)
{
    QString iconFolder = QString(":/icons/images/%1/").arg(iconSet);
    mIcons.insert(ACTION_FILE_NEW, createSVGIcon(iconFolder+"01File-01New.svg",size,size));
    mIcons.insert(ACTION_FILE_OPEN, createSVGIcon(iconFolder+"01File-02Open.svg",size,size));
    mIcons.insert(ACTION_FILE_OPEN_FOLDER, createSVGIcon(iconFolder+"01File-09Open_Folder.svg",size,size));
    mIcons.insert(ACTION_FILE_SAVE, createSVGIcon(iconFolder+"01File-03Save.svg",size,size));
    mIcons.insert(ACTION_FILE_SAVE_AS, createSVGIcon(iconFolder+"01File-04SaveAs.svg",size,size));
    mIcons.insert(ACTION_FILE_SAVE_ALL, createSVGIcon(iconFolder+"01File-05SaveAll.svg",size,size));
    mIcons.insert(ACTION_FILE_CLOSE, createSVGIcon(iconFolder+"01File-06Close.svg",size,size));
    mIcons.insert(ACTION_FILE_CLOSE_ALL, createSVGIcon(iconFolder+"01File-07CloseAll.svg",size,size));
    mIcons.insert(ACTION_FILE_PRINT, createSVGIcon(iconFolder+"01File-08Print.svg",size,size));
    mIcons.insert(ACTION_PROJECT_NEW, createSVGIcon(iconFolder+"02Project_01New.svg",size,size));
    mIcons.insert(ACTION_PROJECT_SAVE, createSVGIcon(iconFolder+"02Project_02Save.svg",size,size));
    mIcons.insert(ACTION_PROJECT_CLOSE, createSVGIcon(iconFolder+"02Project_03Close.svg",size,size));
    mIcons.insert(ACTION_EDIT_UNDO, createSVGIcon(iconFolder+"03Edit_01Undo.svg",size,size));
    mIcons.insert(ACTION_EDIT_REDO, createSVGIcon(iconFolder+"03Edit_02Redo.svg",size,size));
    mIcons.insert(ACTION_EDIT_CUT, createSVGIcon(iconFolder+"03Edit_03Cut.svg",size,size));
    mIcons.insert(ACTION_EDIT_COPY, createSVGIcon(iconFolder+"03Edit_04Copy.svg",size,size));
    mIcons.insert(ACTION_EDIT_PASTE, createSVGIcon(iconFolder+"03Edit_05Paste.svg",size,size));
    mIcons.insert(ACTION_EDIT_INDENT, createSVGIcon(iconFolder+"03Edit_06Indent.svg",size,size));
    mIcons.insert(ACTION_EDIT_UNINDENT, createSVGIcon(iconFolder+"03Edit_07Unindent.svg",size,size));
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
