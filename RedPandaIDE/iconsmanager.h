#ifndef ICONSMANAGER_H
#define ICONSMANAGER_H

#include <QObject>
#include <QPixmap>
#include <memory>

using PIcon = std::shared_ptr<QPixmap>;
class IconsManager : public QObject
{
    Q_OBJECT
public:
    explicit IconsManager(QObject *parent = nullptr);

    const PIcon &syntaxError() const;

    const PIcon &syntaxWarning() const;

    const PIcon &breakpoint() const;

    const PIcon &activeBreakpoint() const;

    const PIcon &bookmark() const;

    const PIcon &folder() const;
signals:
private:
    PIcon mSyntaxError;
    PIcon mSyntaxWarning;
    PIcon mBreakpoint;
    PIcon mActiveBreakpoint;
    PIcon mBookmark;
    PIcon mFolder;
};

extern IconsManager* pIconsManager;
#endif // ICONSMANAGER_H
