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
#include "iconsmanager.h"

#include <QPainter>
#include <QSvgRenderer>
#include <QDebug>
#include <QIcon>
#include <QToolButton>
#include <QPushButton>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include "utils.h"
#include "settings.h"
#include "widgets/customdisablediconengine.h"
#include <QApplication>

IconsManager* pIconsManager;

IconsManager::IconsManager(QObject *parent) : QObject(parent)
{
    mDefaultIconPixmap = std::make_shared<QPixmap>();
    mIconSetTemplate = "%1/%2/%3/";
    mMakeDisabledIconDarker = false;
}

void IconsManager::updateEditorGuttorIcons(const QString& iconSet,int size)
{
    QString iconFolder = mIconSetTemplate.arg( iconSetsFolder(),iconSet,"editor");
    updateMakeDisabledIconDarker(iconSet);
    mIconPixmaps.insert(GUTTER_BREAKPOINT, createSVGIcon(iconFolder+"breakpoint.svg",size,size));
    mIconPixmaps.insert(GUTTER_SYNTAX_ERROR, createSVGIcon(iconFolder+"syntaxerror.svg",size,size));
    mIconPixmaps.insert(GUTTER_SYNTAX_WARNING,createSVGIcon(iconFolder+"syntaxwarning.svg",size,size));
    mIconPixmaps.insert(GUTTER_ACTIVEBREAKPOINT,createSVGIcon(iconFolder+"currentline.svg",size,size));
    mIconPixmaps.insert(GUTTER_BOOKMARK, createSVGIcon(iconFolder+"bookmark.svg",size,size));
}

void IconsManager::updateParserIcons(const QString &iconSet, int size)
{
    QString iconFolder = mIconSetTemplate.arg( iconSetsFolder(),iconSet,"classparser");
    updateMakeDisabledIconDarker(iconSet);
    mIconPixmaps.insert(PARSER_TYPE, createSVGIcon(iconFolder+"type.svg",size,size));
    mIconPixmaps.insert(PARSER_CLASS, createSVGIcon(iconFolder+"class.svg",size,size));
    mIconPixmaps.insert(PARSER_NAMESPACE, createSVGIcon(iconFolder+"namespace.svg",size,size));
    mIconPixmaps.insert(PARSER_DEFINE, createSVGIcon(iconFolder+"define.svg",size,size));
    mIconPixmaps.insert(PARSER_ENUM, createSVGIcon(iconFolder+"enum.svg",size,size));;
    mIconPixmaps.insert(PARSER_GLOBAL_METHOD, createSVGIcon(iconFolder+"global_method.svg",size,size));
    mIconPixmaps.insert(PARSER_INHERITED_PROTECTED_METHOD, createSVGIcon(iconFolder+"method_inherited_protected.svg",size,size));
    mIconPixmaps.insert(PARSER_INHERITED_METHOD, createSVGIcon(iconFolder+"method_inherited.svg",size,size));
    mIconPixmaps.insert(PARSER_PROTECTED_METHOD, createSVGIcon(iconFolder+"method_protected.svg",size,size));
    mIconPixmaps.insert(PARSER_PUBLIC_METHOD, createSVGIcon(iconFolder+"method_public.svg",size,size));
    mIconPixmaps.insert(PARSER_PRIVATE_METHOD, createSVGIcon(iconFolder+"method_private.svg",size,size));
    mIconPixmaps.insert(PARSER_GLOBAL_VAR, createSVGIcon(iconFolder+"global.svg",size,size));
    mIconPixmaps.insert(PARSER_INHERITED_PROTECTD_VAR, createSVGIcon(iconFolder+"var_inherited_protected.svg",size,size));
    mIconPixmaps.insert(PARSER_INHERITED_VAR, createSVGIcon(iconFolder+"var_inherited.svg",size,size));
    mIconPixmaps.insert(PARSER_PROTECTED_VAR, createSVGIcon(iconFolder+"var_protected.svg",size,size));
    mIconPixmaps.insert(PARSER_PUBLIC_VAR, createSVGIcon(iconFolder+"var_public.svg",size,size));
    mIconPixmaps.insert(PARSER_PRIVATE_VAR, createSVGIcon(iconFolder+"var_private.svg",size,size));
    mIconPixmaps.insert(PARSER_KEYWORD, createSVGIcon(iconFolder+"keyword.svg",size,size));
    mIconPixmaps.insert(PARSER_CODE_SNIPPET, createSVGIcon(iconFolder+"code_snippet.svg",size,size));
    mIconPixmaps.insert(PARSER_LOCAL_VAR, createSVGIcon(iconFolder+"var.svg",size,size));

}

void IconsManager::updateActionIcons(const QString& iconSet, int size)
{
    QString iconFolder = mIconSetTemplate.arg(iconSetsFolder(),iconSet,"actions");
    updateMakeDisabledIconDarker(iconSet);
    mActionIconSize = QSize(size,size);
    mIconPixmaps.insert(ACTION_MISC_BACK, createSVGIcon(iconFolder+"00Misc-01Back.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_FORWARD, createSVGIcon(iconFolder+"00Misc-02Forward.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_ADD, createSVGIcon(iconFolder+"00Misc-03Add.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_REMOVE, createSVGIcon(iconFolder+"00Misc-04Remove.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_GEAR, createSVGIcon(iconFolder+"00Misc-05Gear.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_CROSS, createSVGIcon(iconFolder+"00Misc-06Cross.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_FOLDER, createSVGIcon(iconFolder+"00Misc-07Folder.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_TERM, createSVGIcon(iconFolder+"00Misc-08Term.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_CLEAN, createSVGIcon(iconFolder+"00Misc-09Clean.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_VALIDATE, createSVGIcon(iconFolder+"00Misc-10Check.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_RENAME, createSVGIcon(iconFolder+"00Misc-11Rename.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_HELP, createSVGIcon(iconFolder+"00Misc-12Help.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_FILTER, createSVGIcon(iconFolder+"00Misc-13Filter.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_MOVEUP, createSVGIcon(iconFolder+"00Misc-14MoveUp.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_MOVEDOWN, createSVGIcon(iconFolder+"00Misc-15MoveDown.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_RESET, createSVGIcon(iconFolder+"00Misc-16Reset.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_MOVETOP, createSVGIcon(iconFolder+"00Misc-17MoveTop.svg",size,size));
    mIconPixmaps.insert(ACTION_MISC_MOVEBOTTOM, createSVGIcon(iconFolder+"00Misc-18MoveBottom.svg",size,size));

    mIconPixmaps.insert(ACTION_FILE_NEW, createSVGIcon(iconFolder+"01File-01New.svg",size,size));
    mIconPixmaps.insert(ACTION_FILE_OPEN, createSVGIcon(iconFolder+"01File-02Open.svg",size,size));
    mIconPixmaps.insert(ACTION_FILE_OPEN_FOLDER, createSVGIcon(iconFolder+"01File-09Open_Folder.svg",size,size));
    mIconPixmaps.insert(ACTION_FILE_SAVE, createSVGIcon(iconFolder+"01File-03Save.svg",size,size));
    mIconPixmaps.insert(ACTION_FILE_SAVE_AS, createSVGIcon(iconFolder+"01File-04SaveAs.svg",size,size));
    mIconPixmaps.insert(ACTION_FILE_SAVE_ALL, createSVGIcon(iconFolder+"01File-05SaveAll.svg",size,size));
    mIconPixmaps.insert(ACTION_FILE_CLOSE, createSVGIcon(iconFolder+"01File-06Close.svg",size,size));
    mIconPixmaps.insert(ACTION_FILE_CLOSE_ALL, createSVGIcon(iconFolder+"01File-07CloseAll.svg",size,size));
    mIconPixmaps.insert(ACTION_FILE_PRINT, createSVGIcon(iconFolder+"01File-08Print.svg",size,size));
    mIconPixmaps.insert(ACTION_FILE_PROPERTIES, createSVGIcon(iconFolder+"01File-10FileProperties.svg",size,size));
    mIconPixmaps.insert(ACTION_FILE_LOCATE, createSVGIcon(iconFolder+"01File-11Locate.svg",size,size));

    mIconPixmaps.insert(ACTION_PROJECT_NEW, createSVGIcon(iconFolder+"02Project-01New.svg",size,size));
    mIconPixmaps.insert(ACTION_PROJECT_SAVE, createSVGIcon(iconFolder+"02Project-02Save.svg",size,size));
    mIconPixmaps.insert(ACTION_PROJECT_CLOSE, createSVGIcon(iconFolder+"02Project-03Close.svg",size,size));
    mIconPixmaps.insert(ACTION_PROJECT_NEW_FILE, createSVGIcon(iconFolder+"02Project-04NewFile.svg",size,size));
    mIconPixmaps.insert(ACTION_PROJECT_ADD_FILE, createSVGIcon(iconFolder+"02Project-05AddFile.svg",size,size));
    mIconPixmaps.insert(ACTION_PROJECT_REMOVE_FILE, createSVGIcon(iconFolder+"02Project-06RemoveFile.svg",size,size));
    mIconPixmaps.insert(ACTION_PROJECT_PROPERTIES, createSVGIcon(iconFolder+"02Project-07Properties.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_UNDO, createSVGIcon(iconFolder+"03Edit-01Undo.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_REDO, createSVGIcon(iconFolder+"03Edit-02Redo.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_CUT, createSVGIcon(iconFolder+"03Edit-03Cut.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_COPY, createSVGIcon(iconFolder+"03Edit-04Copy.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_PASTE, createSVGIcon(iconFolder+"03Edit-05Paste.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_INDENT, createSVGIcon(iconFolder+"03Edit-06Indent.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_UNINDENT, createSVGIcon(iconFolder+"03Edit-07Unindent.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_SEARCH, createSVGIcon(iconFolder+"03Edit-08Search.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_REPLACE, createSVGIcon(iconFolder+"03Edit-09Replace.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_SEARCH_IN_FILES, createSVGIcon(iconFolder+"03Edit-10SearchInFiles.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_SORT_BY_NAME, createSVGIcon(iconFolder+"03Edit-11SortByName.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_SORT_BY_TYPE, createSVGIcon(iconFolder+"03Edit-12SortByType.svg",size,size));
    mIconPixmaps.insert(ACTION_EDIT_SHOW_INHERITED, createSVGIcon(iconFolder+"03Edit-13ShowInherited.svg",size,size));

    mIconPixmaps.insert(ACTION_CODE_BACK, createSVGIcon(iconFolder+"04Code-01Back.svg",size,size));
    mIconPixmaps.insert(ACTION_CODE_FORWARD, createSVGIcon(iconFolder+"04Code-02Forward.svg",size,size));
    mIconPixmaps.insert(ACTION_CODE_ADD_BOOKMARK, createSVGIcon(iconFolder+"04Code-03AddBookmark.svg",size,size));
    mIconPixmaps.insert(ACTION_CODE_REMOVE_BOOKMARK, createSVGIcon(iconFolder+"04Code-04RemoveBookmark.svg",size,size));
    mIconPixmaps.insert(ACTION_CODE_REFORMAT, createSVGIcon(iconFolder+"04Code-05Reformat.svg",size,size));

    mIconPixmaps.insert(ACTION_RUN_COMPILE, createSVGIcon(iconFolder+"05Run-01Compile.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_COMPILE_RUN, createSVGIcon(iconFolder+"05Run-02CompileRun.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_RUN, createSVGIcon(iconFolder+"05Run-03Run.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_REBUILD, createSVGIcon(iconFolder+"05Run-04Rebuild.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_OPTIONS, createSVGIcon(iconFolder+"05Run-05Options.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_DEBUG, createSVGIcon(iconFolder+"05Run-06Debug.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_STEP_OVER, createSVGIcon(iconFolder+"05Run-07StepOver.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_STEP_INTO, createSVGIcon(iconFolder+"05Run-08StepInto.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_STEP_OUT, createSVGIcon(iconFolder+"05Run-08StepOut.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_RUN_TO_CURSOR, createSVGIcon(iconFolder+"05Run-09RunToCursor.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_CONTINUE, createSVGIcon(iconFolder+"05Run-10Continue.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_STOP, createSVGIcon(iconFolder+"05Run-11Stop.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_ADD_WATCH, createSVGIcon(iconFolder+"05Run-12AddWatch.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_REMOVE_WATCH, createSVGIcon(iconFolder+"05Run-13RemoveWatch.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_STEP_OVER_INSTRUCTION, createSVGIcon(iconFolder+"05Run-14StepOverInstruction.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_STEP_INTO_INSTRUCTION, createSVGIcon(iconFolder+"05Run-15StepIntoInstruction.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_INTERRUPT, createSVGIcon(iconFolder+"05Run-16Interrupt.svg",size,size));
    mIconPixmaps.insert(ACTION_RUN_COMPILE_OPTIONS, createSVGIcon(iconFolder+"05Run-17CompilerOptions.svg",size,size));

    mIconPixmaps.insert(ACTION_VIEW_MAXIMUM, createSVGIcon(iconFolder+"06View-01Maximum.svg",size,size));
    mIconPixmaps.insert(ACTION_VIEW_CLASSBROWSER, createSVGIcon(iconFolder+"06View-02ClassBrowser.svg",size,size));
    mIconPixmaps.insert(ACTION_VIEW_FILES, createSVGIcon(iconFolder+"06View-03Files.svg",size,size));
    mIconPixmaps.insert(ACTION_VIEW_COMPILELOG, createSVGIcon(iconFolder+"06View-04CompileLog.svg",size,size));
    mIconPixmaps.insert(ACTION_VIEW_BOOKMARK, createSVGIcon(iconFolder+"06View-05Bookmark.svg",size,size));
    mIconPixmaps.insert(ACTION_VIEW_TODO, createSVGIcon(iconFolder+"06View-06Todo.svg",size,size));

    mIconPixmaps.insert(ACTION_HELP_ABOUT, createSVGIcon(iconFolder+"07Help-01About.svg",size,size));

    mIconPixmaps.insert(ACTION_PROBLEM_PROBLEM, createSVGIcon(iconFolder+"08Problem-01Problem.svg",size,size));
    mIconPixmaps.insert(ACTION_PROBLEM_SET, createSVGIcon(iconFolder+"08Problem-02ProblemSet.svg",size,size));
    mIconPixmaps.insert(ACTION_PROBLEM_PROPERTIES, createSVGIcon(iconFolder+"08Problem-03Properties.svg",size,size));
    mIconPixmaps.insert(ACTION_PROBLEM_EDIT_SOURCE, createSVGIcon(iconFolder+"08Problem-04EditSource.svg",size,size));
    mIconPixmaps.insert(ACTION_PROBLEM_RUN_CASES, createSVGIcon(iconFolder+"08Problem-05RunCases.svg",size,size));
    mIconPixmaps.insert(ACTION_PROBLEM_PASSED, createSVGIcon(iconFolder+"08Problem-06Correct.svg",size,size));
    mIconPixmaps.insert(ACTION_PROBLEM_FALIED, createSVGIcon(iconFolder+"08Problem-07Wrong.svg",size,size));
    mIconPixmaps.insert(ACTION_PROBLEM_TESTING, createSVGIcon(iconFolder+"08Problem-08Running.svg",size,size));

    emit actionIconsUpdated();

}

void IconsManager::updateFileSystemIcons(const QString &iconSet, int size)
{
    QString iconFolder = mIconSetTemplate.arg( iconSetsFolder(),iconSet,"filesystem");
    updateMakeDisabledIconDarker(iconSet);
    mIconPixmaps.insert(FILESYSTEM_GIT, createSVGIcon(iconFolder+"git.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_FOLDER, createSVGIcon(iconFolder+"folder.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_FOLDER_VCS_CHANGED, createSVGIcon(iconFolder+"folder-vcs-changed.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_FOLDER_VCS_CONFLICT, createSVGIcon(iconFolder+"folder-vcs-conflict.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_FOLDER_VCS_NOCHANGE, createSVGIcon(iconFolder+"folder-vcs-nochange.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_FOLDER_VCS_STAGED, createSVGIcon(iconFolder+"folder-vcs-staged.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_FILE, createSVGIcon(iconFolder+"file.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_FILE_VCS_CHANGED, createSVGIcon(iconFolder+"file-vcs-changed.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_FILE_VCS_CONFLICT, createSVGIcon(iconFolder+"file-vcs-conflict.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_FILE_VCS_NOCHANGE, createSVGIcon(iconFolder+"file-vcs-nochange.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_FILE_VCS_STAGED, createSVGIcon(iconFolder+"file-vcs-staged.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_CFILE, createSVGIcon(iconFolder+"cfile.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_CFILE_VCS_CHANGED, createSVGIcon(iconFolder+"cfile-vcs-changed.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_CFILE_VCS_CONFLICT, createSVGIcon(iconFolder+"cfile-vcs-conflict.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_CFILE_VCS_NOCHANGE, createSVGIcon(iconFolder+"cfile-vcs-nochange.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_CFILE_VCS_STAGED, createSVGIcon(iconFolder+"cfile-vcs-staged.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_HFILE, createSVGIcon(iconFolder+"hfile.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_HFILE_VCS_CHANGED, createSVGIcon(iconFolder+"hfile-vcs-changed.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_HFILE_VCS_CONFLICT, createSVGIcon(iconFolder+"hfile-vcs-conflict.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_HFILE_VCS_NOCHANGE, createSVGIcon(iconFolder+"hfile-vcs-nochange.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_HFILE_VCS_STAGED, createSVGIcon(iconFolder+"hfile-vcs-staged.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_CPPFILE, createSVGIcon(iconFolder+"cppfile.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_CPPFILE_VCS_CHANGED, createSVGIcon(iconFolder+"cppfile-vcs-changed.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_CPPFILE_VCS_CONFLICT, createSVGIcon(iconFolder+"cppfile-vcs-conflict.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_CPPFILE_VCS_NOCHANGE, createSVGIcon(iconFolder+"cppfile-vcs-nochange.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_CPPFILE_VCS_STAGED, createSVGIcon(iconFolder+"cppfile-vcs-staged.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_PROJECTFILE, createSVGIcon(iconFolder+"projectfile.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_PROJECTFILE_VCS_CHANGED, createSVGIcon(iconFolder+"projectfile-vcs-changed.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_PROJECTFILE_VCS_CONFLICT, createSVGIcon(iconFolder+"projectfile-vcs-conflict.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_PROJECTFILE_VCS_NOCHANGE, createSVGIcon(iconFolder+"projectfile-vcs-nochange.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_PROJECTFILE_VCS_STAGED, createSVGIcon(iconFolder+"projectfile-vcs-staged.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_HEADERS_FOLDER, createSVGIcon(iconFolder+"headerfolder.svg",size,size));
    mIconPixmaps.insert(FILESYSTEM_SOURCES_FOLDER, createSVGIcon(iconFolder+"sourcefolder.svg",size,size));
}

IconsManager::PPixmap IconsManager::getPixmap(IconName iconName) const
{
    return mIconPixmaps.value(iconName, mDefaultIconPixmap);
}

QIcon IconsManager:: getIcon(IconName iconName) const
{
    PPixmap pixmap = getPixmap(iconName);
    if (pixmap == mDefaultIconPixmap)
        return QIcon();
    if (mMakeDisabledIconDarker) {
        //QIcon takes the owner ship
        QIcon icon(new CustomDisabledIconEngine);
        icon.addPixmap(*pixmap);
        return icon;
    } else
        return QIcon(*pixmap);
}

void IconsManager::setIcon(QToolButton *btn, IconName iconName) const
{
    btn->setIconSize(mActionIconSize);
    btn->setIcon(getIcon(iconName));
}

void IconsManager::setIcon(QPushButton *btn, IconName iconName) const
{
    btn->setIconSize(mActionIconSize);
    btn->setIcon(getIcon(iconName));
}

IconsManager::PPixmap IconsManager::createSVGIcon(const QString &filename, int width, int height)
{
    QSvgRenderer renderer(filename);
    if (!renderer.isValid())
        return mDefaultIconPixmap;
    qreal dpr=qApp->devicePixelRatio();
    PPixmap icon = std::make_shared<QPixmap>(width*dpr,height*dpr);
    icon->fill(Qt::transparent);
    QPainter painter(icon.get());
    renderer.render(&painter,icon->rect());
    icon->setDevicePixelRatio(dpr);
    return icon;
}

const QSize &IconsManager::actionIconSize() const
{
    return mActionIconSize;
}

void IconsManager::prepareCustomIconSet(const QString &customIconSet)
{
    if (QFile(customIconSet).exists())
        return;
    copyFolder(pSettings->dirs().data(Settings::Dirs::DataType::IconSet),customIconSet);
}

QPixmap IconsManager::getPixmapForStatement(PStatement statement)
{
    if (!statement)
        return QPixmap();
    StatementKind kind = getKindOfStatement(statement);
    switch (kind) {
    case StatementKind::Typedef:
        return *(pIconsManager->getPixmap(IconsManager::PARSER_TYPE));
    case StatementKind::Class:
        return *(pIconsManager->getPixmap(IconsManager::PARSER_CLASS));
    case StatementKind::Namespace:
    case StatementKind::NamespaceAlias:
        return *(pIconsManager->getPixmap(IconsManager::PARSER_NAMESPACE));
    case StatementKind::Preprocessor:
        return *(pIconsManager->getPixmap(IconsManager::PARSER_DEFINE));
    case StatementKind::EnumClassType:
    case StatementKind::EnumType:
    case StatementKind::Enum:
        return *(pIconsManager->getPixmap(IconsManager::PARSER_ENUM));
    case StatementKind::Function:
    case StatementKind::Constructor:
    case StatementKind::Destructor:
        if (statement->scope == StatementScope::Global)
            return *(pIconsManager->getPixmap(IconsManager::PARSER_GLOBAL_METHOD));
        if (statement->isInherited()) {
            if (statement->accessibility == StatementAccessibility::Protected) {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_INHERITED_PROTECTED_METHOD));
            } else if (statement->accessibility == StatementAccessibility::Public) {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_INHERITED_METHOD));
            } else {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_PRIVATE_METHOD));
            }
        } else {
            if (statement->accessibility == StatementAccessibility::Protected) {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_PROTECTED_METHOD));
            } else if (statement->accessibility == StatementAccessibility::Public) {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_PUBLIC_METHOD));
            } else {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_PRIVATE_METHOD));
            }
        }
        break;
    case StatementKind::GlobalVariable:
        return *(pIconsManager->getPixmap(IconsManager::PARSER_GLOBAL_VAR));
    case StatementKind::LocalVariable:
        return *(pIconsManager->getPixmap(IconsManager::PARSER_LOCAL_VAR));
    case StatementKind::Variable:
        if (statement->isInherited()) {
            if (statement->accessibility == StatementAccessibility::Protected) {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_INHERITED_PROTECTD_VAR));
            } else if (statement->accessibility == StatementAccessibility::Public) {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_INHERITED_VAR));
            } else {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_PRIVATE_VAR));
            }
        } else {
            if (statement->accessibility == StatementAccessibility::Protected) {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_PROTECTED_VAR));
            } else if (statement->accessibility == StatementAccessibility::Public) {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_PUBLIC_VAR));
            } else {
                return *(pIconsManager->getPixmap(IconsManager::PARSER_PRIVATE_VAR));
            }
        }
        break;
    case StatementKind::Keyword:
        return *(pIconsManager->getPixmap(IconsManager::PARSER_KEYWORD));
    case StatementKind::UserCodeSnippet:
        return *(pIconsManager->getPixmap(IconsManager::PARSER_CODE_SNIPPET));
    case StatementKind::Alias:
        return *(pIconsManager->getPixmap(IconsManager::PARSER_TYPE));
    default:
        break;
    }
    return QPixmap();
}

const QString IconsManager::iconSetsFolder() const
{
    if (mIconSetsFolder.isEmpty())
        return pSettings->dirs().data(Settings::Dirs::DataType::IconSet);
    return mIconSetsFolder;
}

void IconsManager::setIconSetsFolder(const QString &newIconSetsFolder)
{
    mIconSetsFolder = newIconSetsFolder;
}

QList<PIconSet> IconsManager::listIconSets()
{
    QDirIterator dirIter(iconSetsFolder());
    QList<PIconSet> result;
    while(dirIter.hasNext()) {
        dirIter.next();
        QFileInfo fileInfo = dirIter.fileInfo();
        if (!fileInfo.isHidden() && !fileInfo.fileName().startsWith('.') && fileInfo.isDir()) {
            PIconSet pSet = std::make_shared<IconSet>();
            pSet->name = fileInfo.baseName();
            pSet->displayName = pSet->name;
            QFile infoFile(includeTrailingPathDelimiter(fileInfo.absoluteFilePath())+"info.json");
            if (infoFile.exists() && infoFile.open(QFile::ReadOnly)) {
                QByteArray content = infoFile.readAll().trimmed();
                if (content.isEmpty())
                    return result;
                QJsonParseError error;
                QJsonDocument doc(QJsonDocument::fromJson(content,&error));
                if (error.error  == QJsonParseError::NoError) {
                    QJsonObject obj=doc.object();
                    pSet->displayName = obj["name"].toString();
                    QString localeName = obj["name_"+pSettings->environment().language()].toString();
                    if (!localeName.isEmpty())
                        pSet->displayName = localeName;
                }
            }
            result.append(pSet);
        }
    }
    return result;
}

void IconsManager::updateMakeDisabledIconDarker(const QString& iconset )
{
    mMakeDisabledIconDarker = (iconset == "contrast");
}
