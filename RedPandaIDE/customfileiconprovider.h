#ifndef CUSTOMFILEICONPROVIDER_H
#define CUSTOMFILEICONPROVIDER_H

#include <QFileIconProvider>

class CustomFileIconProvider : public QFileIconProvider
{
public:
    CustomFileIconProvider();

    // QFileIconProvider interface
public:
    QIcon icon(IconType type) const override;
    QIcon icon(const QFileInfo &info) const override;
};

#endif // CUSTOMFILEICONPROVIDER_H
