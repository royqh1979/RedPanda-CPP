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

        ACTION_FILE_NEW,
        ACTION_FILE_OPEN,
        ACTION_FILE_OPEN_FOLDER,
        ACTION_FILE_SAVE,
        ACTION_FILE_SAVE_AS,
        ACTION_FILE_SAVE_ALL,
        ACTION_FILE_CLOSE,
        ACTION_FILE_CLOSE_ALL,
        ACTION_FILE_PRINT,
        ACTION_PROJECT_NEW,
        ACTION_PROJECT_SAVE,
        ACTION_PROJECT_CLOSE,

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
