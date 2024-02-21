#ifndef FILENAMEDELEGATE_H
#define FILENAMEDELEGATE_H

#include <QStyledItemDelegate>

class FileNameDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    FileNameDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // FILENAMEDELEGATE_H
