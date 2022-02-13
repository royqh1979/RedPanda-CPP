#ifndef CUSTOMFILEICONPROVIDER_H
#define CUSTOMFILEICONPROVIDER_H

#include <QFileIconProvider>

class GitManager;
class CustomFileIconProvider : public QFileIconProvider
{
public:
    CustomFileIconProvider();
    ~CustomFileIconProvider();
private:
    GitManager* mVCSManager;
    // QFileIconProvider interface
public:
    QIcon icon(IconType type) const override;
    QIcon icon(const QFileInfo &info) const override;
};

#endif // CUSTOMFILEICONPROVIDER_H
