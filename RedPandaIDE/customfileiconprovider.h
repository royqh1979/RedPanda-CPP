#ifndef CUSTOMFILEICONPROVIDER_H
#define CUSTOMFILEICONPROVIDER_H

#include <QFileIconProvider>

class GitRepository;
class CustomFileIconProvider : public QFileIconProvider
{
public:
    CustomFileIconProvider();
    ~CustomFileIconProvider();
    void setRootFolder(const QString& folder);
    void update();
private:
    GitRepository* mVCSRepository;
    // QFileIconProvider interface
public:
    QIcon icon(IconType type) const override;
    QIcon icon(const QFileInfo &info) const override;
};

#endif // CUSTOMFILEICONPROVIDER_H
