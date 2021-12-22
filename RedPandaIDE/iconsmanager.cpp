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
    mIcons.insert(ACTION_MISC_BACK, createSVGIcon(iconFolder+"00Misc-01Back.svg",size,size));
    mIcons.insert(ACTION_MISC_FORWARD, createSVGIcon(iconFolder+"00Misc-02Forward.svg",size,size));
    mIcons.insert(ACTION_MISC_ADD, createSVGIcon(iconFolder+"00Misc-03Add.svg",size,size));
    mIcons.insert(ACTION_MISC_REMOVE, createSVGIcon(iconFolder+"00Misc-04Remove.svg",size,size));
    mIcons.insert(ACTION_MISC_GEAR, createSVGIcon(iconFolder+"00Misc-05Gear.svg",size,size));
    mIcons.insert(ACTION_MISC_CROSS, createSVGIcon(iconFolder+"00Misc-06Cross.svg",size,size));
    mIcons.insert(ACTION_MISC_FOLDER, createSVGIcon(iconFolder+"00Misc-07Folder.svg",size,size));
    mIcons.insert(ACTION_MISC_TERM, createSVGIcon(iconFolder+"00Misc-08Term.svg",size,size));
    mIcons.insert(ACTION_MISC_CLEAN, createSVGIcon(iconFolder+"00Misc-09Clean.svg",size,size));

    mIcons.insert(ACTION_FILE_NEW, createSVGIcon(iconFolder+"01File-01New.svg",size,size));
    mIcons.insert(ACTION_FILE_OPEN, createSVGIcon(iconFolder+"01File-02Open.svg",size,size));
    mIcons.insert(ACTION_FILE_OPEN_FOLDER, createSVGIcon(iconFolder+"01File-09Open_Folder.svg",size,size));
    mIcons.insert(ACTION_FILE_SAVE, createSVGIcon(iconFolder+"01File-03Save.svg",size,size));
    mIcons.insert(ACTION_FILE_SAVE_AS, createSVGIcon(iconFolder+"01File-04SaveAs.svg",size,size));
    mIcons.insert(ACTION_FILE_SAVE_ALL, createSVGIcon(iconFolder+"01File-05SaveAll.svg",size,size));
    mIcons.insert(ACTION_FILE_CLOSE, createSVGIcon(iconFolder+"01File-06Close.svg",size,size));
    mIcons.insert(ACTION_FILE_CLOSE_ALL, createSVGIcon(iconFolder+"01File-07CloseAll.svg",size,size));
    mIcons.insert(ACTION_FILE_PRINT, createSVGIcon(iconFolder+"01File-08Print.svg",size,size));
    mIcons.insert(ACTION_FILE_PROPERTIES, createSVGIcon(iconFolder+"01File-10FileProperties.svg",size,size));
    mIcons.insert(ACTION_FILE_LOCATE, createSVGIcon(iconFolder+"01File-11Locate.svg",size,size));

    mIcons.insert(ACTION_PROJECT_NEW, createSVGIcon(iconFolder+"02Project_01New.svg",size,size));
    mIcons.insert(ACTION_PROJECT_SAVE, createSVGIcon(iconFolder+"02Project_02Save.svg",size,size));
    mIcons.insert(ACTION_PROJECT_CLOSE, createSVGIcon(iconFolder+"02Project_03Close.svg",size,size));
    mIcons.insert(ACTION_PROJECT_NEW_FILE, createSVGIcon(iconFolder+"02Project_04NewFile.svg",size,size));
    mIcons.insert(ACTION_PROJECT_ADD_FILE, createSVGIcon(iconFolder+"02Project_05AddFile.svg",size,size));
    mIcons.insert(ACTION_PROJECT_REMOVE_FILE, createSVGIcon(iconFolder+"02Project_06RemoveFile.svg",size,size));
    mIcons.insert(ACTION_PROJECT_PROPERTIES, createSVGIcon(iconFolder+"02Project_07Properties.svg",size,size));
    mIcons.insert(ACTION_EDIT_UNDO, createSVGIcon(iconFolder+"03Edit_01Undo.svg",size,size));
    mIcons.insert(ACTION_EDIT_REDO, createSVGIcon(iconFolder+"03Edit_02Redo.svg",size,size));
    mIcons.insert(ACTION_EDIT_CUT, createSVGIcon(iconFolder+"03Edit_03Cut.svg",size,size));
    mIcons.insert(ACTION_EDIT_COPY, createSVGIcon(iconFolder+"03Edit_04Copy.svg",size,size));
    mIcons.insert(ACTION_EDIT_PASTE, createSVGIcon(iconFolder+"03Edit_05Paste.svg",size,size));
    mIcons.insert(ACTION_EDIT_INDENT, createSVGIcon(iconFolder+"03Edit_06Indent.svg",size,size));
    mIcons.insert(ACTION_EDIT_UNINDENT, createSVGIcon(iconFolder+"03Edit_07Unindent.svg",size,size));
    mIcons.insert(ACTION_EDIT_SEARCH, createSVGIcon(iconFolder+"03Edit_08Search.svg",size,size));
    mIcons.insert(ACTION_EDIT_REPLACE, createSVGIcon(iconFolder+"03Edit_09Replace.svg",size,size));
    mIcons.insert(ACTION_EDIT_SEARCH_IN_FILES, createSVGIcon(iconFolder+"03Edit_10SearchInFiles.svg",size,size));
    mIcons.insert(ACTION_EDIT_SORT_BY_NAME, createSVGIcon(iconFolder+"03Edit-11SortByName.svg",size,size));
    mIcons.insert(ACTION_EDIT_SORT_BY_TYPE, createSVGIcon(iconFolder+"03Edit-12SortByType.svg",size,size));
    mIcons.insert(ACTION_EDIT_SHOW_INHERITED, createSVGIcon(iconFolder+"03Edit-13ShowInherited.svg",size,size));

    mIcons.insert(ACTION_CODE_BACK, createSVGIcon(iconFolder+"04Code-01Back.svg",size,size));
    mIcons.insert(ACTION_CODE_FORWARD, createSVGIcon(iconFolder+"04Code-02Forward.svg",size,size));
    mIcons.insert(ACTION_CODE_ADD_BOOKMARK, createSVGIcon(iconFolder+"04Code-03AddBookmark.svg",size,size));
    mIcons.insert(ACTION_CODE_REMOVE_BOOKMARK, createSVGIcon(iconFolder+"04Code-04RemoveBookmark.svg",size,size));
    mIcons.insert(ACTION_CODE_REFORMAT, createSVGIcon(iconFolder+"04Code-05Reformat.svg",size,size));

    mIcons.insert(ACTION_RUN_COMPILE, createSVGIcon(iconFolder+"05Run-01Compile.svg",size,size));
    mIcons.insert(ACTION_RUN_COMPILE_RUN, createSVGIcon(iconFolder+"05Run-02CompileRun.svg",size,size));
    mIcons.insert(ACTION_RUN_RUN, createSVGIcon(iconFolder+"05Run-03Run.svg",size,size));
    mIcons.insert(ACTION_RUN_REBUILD, createSVGIcon(iconFolder+"05Run-04Rebuild.svg",size,size));
    mIcons.insert(ACTION_RUN_OPTIONS, createSVGIcon(iconFolder+"05Run-05Options.svg",size,size));
    mIcons.insert(ACTION_RUN_DEBUG, createSVGIcon(iconFolder+"05Run-06Debug.svg",size,size));
    mIcons.insert(ACTION_RUN_STEP_OVER, createSVGIcon(iconFolder+"05Run-07StepOver.svg",size,size));
    mIcons.insert(ACTION_RUN_STEP_INTO, createSVGIcon(iconFolder+"05Run-08StepInto.svg",size,size));
    mIcons.insert(ACTION_RUN_STEP_OUT, createSVGIcon(iconFolder+"05Run-08StepOut.svg",size,size));
    mIcons.insert(ACTION_RUN_RUN_TO_CURSOR, createSVGIcon(iconFolder+"05Run-09RunToCursor.svg",size,size));
    mIcons.insert(ACTION_RUN_CONTINUE, createSVGIcon(iconFolder+"05Run-10Continue.svg",size,size));
    mIcons.insert(ACTION_RUN_STOP, createSVGIcon(iconFolder+"05Run-11Stop.svg",size,size));
    mIcons.insert(ACTION_RUN_ADD_WATCH, createSVGIcon(iconFolder+"05Run-12AddWatch.svg",size,size));
    mIcons.insert(ACTION_RUN_REMOVE_WATCH, createSVGIcon(iconFolder+"05Run-13RemoveWatch.svg",size,size));

    mIcons.insert(ACTION_VIEW_MAXIMUM, createSVGIcon(iconFolder+"06View-01Maximum.svg",size,size));
    mIcons.insert(ACTION_VIEW_CLASSBROWSER, createSVGIcon(iconFolder+"06View-02ClassBrowser.svg",size,size));
    mIcons.insert(ACTION_VIEW_FILES, createSVGIcon(iconFolder+"06View-03Files.svg",size,size));
    mIcons.insert(ACTION_VIEW_COMPILELOG, createSVGIcon(iconFolder+"06View-04CompileLog.svg",size,size));
    mIcons.insert(ACTION_VIEW_BOOKMARK, createSVGIcon(iconFolder+"06View-05Bookmark.svg",size,size));
    mIcons.insert(ACTION_VIEW_TODO, createSVGIcon(iconFolder+"06View-06Todo.svg",size,size));

    mIcons.insert(ACTION_HELP_ABOUT, createSVGIcon(iconFolder+"07Help-01About.svg",size,size));

    mIcons.insert(ACTION_PROBLEM_PROBLEM, createSVGIcon(iconFolder+"08Problem_01Problem.svg",size,size));
    mIcons.insert(ACTION_PROBLEM_SET, createSVGIcon(iconFolder+"08Problem_02ProblemSet.svg",size,size));
    mIcons.insert(ACTION_PROBLEM_PROPERTIES, createSVGIcon(iconFolder+"08Problem_03Properties.svg",size,size));
    mIcons.insert(ACTION_PROBLEM_EDIT_SOURCE, createSVGIcon(iconFolder+"08Problem_04EditSource.svg",size,size));
    mIcons.insert(ACTION_PROBLEM_RUN_CASES, createSVGIcon(iconFolder+"08Problem_05RunCases.svg",size,size));

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
