#ifndef ICONSMANAGER_H
#define ICONSMANAGER_H

#include <QMap>
#include <QObject>
#include <QPixmap>
#include <memory>

class IconsManager : public QObject
{
    Q_OBJECT
public:
    using PIcon = std::shared_ptr<QPixmap>;
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

        ACTION_MISC_BACK,
        ACTION_MISC_FORWARD,
        ACTION_MISC_ADD,
        ACTION_MISC_REMOVE,
        ACTION_MISC_GEAR,
        ACTION_MISC_CROSS,
        ACTION_MISC_FOLDER,
        ACTION_MISC_TERM,
        ACTION_MISC_CLEAN,

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
        ACTION_PROBLEM_RUN_CASES
    };
    explicit IconsManager(QObject *parent = nullptr);

    void updateEditorGuttorIcons(const QString& iconSet, int size);
    void updateParserIcons(const QString& iconSet, int size);
    void updateActionIcons(const QString iconSet, int size);

    PIcon getIcon(IconName iconName) const;

    const PIcon &folder() const;

    PIcon createSVGIcon(const QString& filename, int width, int height);
private:
    QMap<IconName,PIcon> mIcons;
    PIcon mDefaultIcon;
    PIcon mFolder;
};

extern IconsManager* pIconsManager;
#endif // ICONSMANAGER_H
