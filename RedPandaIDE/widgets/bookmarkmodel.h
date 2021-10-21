#ifndef BOOKMARKMODEL_H
#define BOOKMARKMODEL_H

#include <QAbstractItemModel>

class BookmarkModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    BookmarkModel(QObject* parent=nullptr);
};

#endif // BOOKMARKMODEL_H
