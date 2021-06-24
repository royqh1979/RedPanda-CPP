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

    PIcon syntaxError() const;

    PIcon syntaxWarning() const;

    PIcon breakpoint() const;

    PIcon currentLine() const;

signals:
private:
    PIcon mSyntaxError;
    PIcon mSyntaxWarning;
    PIcon mBreakpoint;
    PIcon mCurrentLine;
};

extern IconsManager* pIconsManager;
#endif // ICONSMANAGER_H
