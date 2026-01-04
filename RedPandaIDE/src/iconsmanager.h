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
#ifndef ICONSMANAGER_H
#define ICONSMANAGER_H

#include <QMap>
#include <QObject>
#include <QPixmap>
#include <memory>
#include "parser/parserutils.h"

struct IconSet {
    QString name;
    QString displayName;
};

using PIconSet = std::shared_ptr<IconSet>;

class QToolButton;
class QPushButton;
class IconsManager : public QObject
{
    Q_OBJECT
public:
    using PPixmap = std::shared_ptr<QPixmap>;
    enum IconName {
        GUTTER_SYNTAX_ERROR,
        GUTTER_SYNTAX_WARNING,
        GUTTER_BREAKPOINT,
        GUTTER_ACTIVEBREAKPOINT,
        GUTTER_BOOKMARK,

        PARSER_TYPE,
        PARSER_CLASS,
        PARSER_NAMESPACE,
        PARSER_DEFINE,
        PARSER_ENUM,
        PARSER_GLOBAL_METHOD,
        PARSER_INHERITED_PROTECTED_METHOD,
        PARSER_INHERITED_METHOD,
        PARSER_PROTECTED_METHOD,
        PARSER_PUBLIC_METHOD,
        PARSER_PRIVATE_METHOD,
        PARSER_GLOBAL_VAR,
        PARSER_INHERITED_PROTECTD_VAR,
        PARSER_INHERITED_VAR,
        PARSER_PROTECTED_VAR,
        PARSER_PUBLIC_VAR,
        PARSER_PRIVATE_VAR,
        PARSER_KEYWORD,
        PARSER_CODE_SNIPPET,
        PARSER_LOCAL_VAR,

        FILESYSTEM_GIT,
        FILESYSTEM_FOLDER,
        FILESYSTEM_FILE,
        FILESYSTEM_CFILE,
        FILESYSTEM_HFILE,
        FILESYSTEM_PROJECTFILE,
        FILESYSTEM_CPPFILE,
        FILESYSTEM_FOLDER_VCS_CHANGED,
        FILESYSTEM_FILE_VCS_CHANGED,
        FILESYSTEM_CFILE_VCS_CHANGED,
        FILESYSTEM_HFILE_VCS_CHANGED,
        FILESYSTEM_PROJECTFILE_VCS_CHANGED,
        FILESYSTEM_CPPFILE_VCS_CHANGED,
        FILESYSTEM_FOLDER_VCS_CONFLICT,
        FILESYSTEM_FILE_VCS_CONFLICT,
        FILESYSTEM_CFILE_VCS_CONFLICT,
        FILESYSTEM_HFILE_VCS_CONFLICT,
        FILESYSTEM_PROJECTFILE_VCS_CONFLICT,
        FILESYSTEM_CPPFILE_VCS_CONFLICT,
        FILESYSTEM_FOLDER_VCS_NOCHANGE,
        FILESYSTEM_FILE_VCS_NOCHANGE,
        FILESYSTEM_CFILE_VCS_NOCHANGE,
        FILESYSTEM_HFILE_VCS_NOCHANGE,
        FILESYSTEM_PROJECTFILE_VCS_NOCHANGE,
        FILESYSTEM_CPPFILE_VCS_NOCHANGE,
        FILESYSTEM_FOLDER_VCS_STAGED,
        FILESYSTEM_FILE_VCS_STAGED,
        FILESYSTEM_CFILE_VCS_STAGED,
        FILESYSTEM_HFILE_VCS_STAGED,
        FILESYSTEM_PROJECTFILE_VCS_STAGED,
        FILESYSTEM_CPPFILE_VCS_STAGED,
        FILESYSTEM_HEADERS_FOLDER,
        FILESYSTEM_SOURCES_FOLDER,


        ACTION_MISC_BACK,
        ACTION_MISC_FORWARD,
        ACTION_MISC_ADD,
        ACTION_MISC_REMOVE,
        ACTION_MISC_GEAR,
        ACTION_MISC_CROSS,
        ACTION_MISC_FOLDER,
        ACTION_MISC_TERM,
        ACTION_MISC_CLEAN,
        ACTION_MISC_VALIDATE,
        ACTION_MISC_RENAME,
        ACTION_MISC_HELP,
        ACTION_MISC_FILTER,
        ACTION_MISC_MOVEUP,
        ACTION_MISC_MOVEDOWN,
        ACTION_MISC_RESET,
        ACTION_MISC_MOVETOP,
        ACTION_MISC_MOVEBOTTOM,

        ACTION_FILE_NEW,
        ACTION_FILE_OPEN,
        ACTION_FILE_OPEN_FOLDER,
        ACTION_FILE_SAVE,
        ACTION_FILE_SAVE_AS,
        ACTION_FILE_SAVE_ALL,
        ACTION_FILE_CLOSE,
        ACTION_FILE_CLOSE_ALL,
        ACTION_FILE_PRINT,
        ACTION_FILE_PROPERTIES,
        ACTION_FILE_LOCATE,

        ACTION_PROJECT_NEW,
        ACTION_PROJECT_SAVE,
        ACTION_PROJECT_CLOSE,
        ACTION_PROJECT_NEW_FILE,
        ACTION_PROJECT_ADD_FILE,
        ACTION_PROJECT_REMOVE_FILE,
        ACTION_PROJECT_PROPERTIES,

        ACTION_EDIT_UNDO,
        ACTION_EDIT_REDO,
        ACTION_EDIT_CUT,
        ACTION_EDIT_COPY,
        ACTION_EDIT_PASTE,
        ACTION_EDIT_INDENT,
        ACTION_EDIT_UNINDENT,
        ACTION_EDIT_SEARCH,
        ACTION_EDIT_REPLACE,
        ACTION_EDIT_SEARCH_IN_FILES,
        ACTION_EDIT_SORT_BY_NAME,
        ACTION_EDIT_SORT_BY_TYPE,
        ACTION_EDIT_SHOW_INHERITED,

        ACTION_CODE_BACK,
        ACTION_CODE_FORWARD,
        ACTION_CODE_ADD_BOOKMARK,
        ACTION_CODE_REMOVE_BOOKMARK,
        ACTION_CODE_REFORMAT,

        ACTION_RUN_COMPILE,
        ACTION_RUN_COMPILE_RUN,
        ACTION_RUN_RUN,
        ACTION_RUN_REBUILD,
        ACTION_RUN_OPTIONS,
        ACTION_RUN_DEBUG,
        ACTION_RUN_STEP_OVER,
        ACTION_RUN_STEP_INTO,
        ACTION_RUN_STEP_OUT,
        ACTION_RUN_RUN_TO_CURSOR,
        ACTION_RUN_CONTINUE,
        ACTION_RUN_STOP,
        ACTION_RUN_ADD_WATCH,
        ACTION_RUN_REMOVE_WATCH,
        ACTION_RUN_STEP_OVER_INSTRUCTION,
        ACTION_RUN_STEP_INTO_INSTRUCTION,
        ACTION_RUN_INTERRUPT,
        ACTION_RUN_COMPILE_OPTIONS,

        ACTION_VIEW_MAXIMUM,
        ACTION_VIEW_CLASSBROWSER,
        ACTION_VIEW_FILES,
        ACTION_VIEW_COMPILELOG,
        ACTION_VIEW_BOOKMARK,
        ACTION_VIEW_TODO,

        ACTION_HELP_ABOUT,

        ACTION_PROBLEM_PROBLEM,
        ACTION_PROBLEM_SET,
        ACTION_PROBLEM_PROPERTIES,
        ACTION_PROBLEM_EDIT_SOURCE,
        ACTION_PROBLEM_RUN_CASES,
        ACTION_PROBLEM_PASSED,
        ACTION_PROBLEM_FALIED,
        ACTION_PROBLEM_TESTING
    };
    explicit IconsManager(QObject *parent = nullptr);

    void updateEditorGutterIcons(const QString& iconSet, int size);
    void updateParserIcons(const QString& iconSet, int size);
    void updateActionIcons(const QString& iconSet, int size);
    void updateFileSystemIcons(const QString& iconSet, int size);

    PPixmap getPixmap(IconName iconName) const;

    QIcon getIcon(IconName iconName) const;

    void setIcon(QToolButton* btn, IconName iconName) const;
    void setIcon(QPushButton* btn, IconName iconName) const;

    PPixmap createSVGIcon(const QString& filename, int width, int height);
    const QSize &actionIconSize() const;

    void prepareCustomIconSet(const QString &customIconSet);

    QPixmap getPixmapForStatement(PStatement statement);
    QPixmap getPixmapForStatement(PStatement statement, int size);

    const QString iconSetsFolder() const;
    void setIconSetsFolder(const QString &newIconSetsFolder);

    QList<PIconSet> listIconSets();
    QString iconSet() const;
    void setIconSet(const QString &newIconSet);

private:
    void updateMakeDisabledIconDarker(const QString& iconset);
    void updateParserIcons(QMap<IconName,PPixmap> &iconPixmaps, const QString& iconSet, int size);
    QPixmap getPixmapForStatement(const QMap<IconName,PPixmap> &iconPixmaps, PStatement statement);
    PPixmap getPixmap(const QMap<IconName,PPixmap> &iconPixmaps, IconName iconName) const;
signals:
    void actionIconsUpdated();
private:
    QMap<IconName,PPixmap> mIconPixmaps;
    PPixmap mDefaultIconPixmap;
    QSize mActionIconSize;
    QString mIconSetTemplate;
    QString mIconSetsFolder;
    QString mParserIconSet;
    int mParserIconSize;
    QString mCachedParserIconSet;
    int mCachedParserIconSize;
    QMap<IconName,PPixmap> mCachedParserIconPixmaps;

    bool mMakeDisabledIconDarker;
};

extern IconsManager* pIconsManager;
#endif // ICONSMANAGER_H
