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
        PARSER_TYPE, //":/icons/images/classparser/type.svg"
        PARSER_CLASS, //":/icons/images/classparser/class.svg"
        PARSER_NAMESPACE, //":/icons/images/classparser/namespace.svg"
        PARSER_DEFINE, // ":/icons/images/classparser/define.svg"
        PARSER_ENUM, // ":/icons/images/classparser/enum.svg";
        PARSER_GLOBAL_METHOD, // ":/icons/images/classparser/global_method.svg"
        PARSER_INHERITED_PROTECTED_METHOD, //":/icons/images/classparser/method_inherited_protected.svg"
        PARSER_INHERITED_METHOD, // ":/icons/images/classparser/method_inherited.svg"
        PARSER_PROTECTED_METHOD, // ":/icons/images/classparser/method_protected.svg"
        PARSER_PUBLIC_METHOD, //":/icons/images/classparser/method_public.svg")
        PARSER_PRIVATE_METHOD, //":/icons/images/classparser/method_private.svg"
        PARSER_GLOBAL_VAR, // ":/icons/images/classparser/global.svg"
        PARSER_INHERITED_PROTECTD_VAR, //":/icons/images/classparser/var_inherited_protected.svg"
        PARSER_INHERITED_VAR, //":/icons/images/classparser/var_inherited.svg"
        PARSER_PROTECTED_VAR, //":/icons/images/classparser/var_protected.svg"
        PARSER_PUBLIC_VAR, //":/icons/images/classparser/var_public.svg"
        PARSER_PRIVATE_VAR //":/icons/images/classparser/var_private.svg"
    };
    explicit IconsManager(QObject *parent = nullptr);

    void updateEditorGuttorIcons(const QString& iconSet, int size);
    void updateParserIcons(const QString& iconSet, int size);

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
