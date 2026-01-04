#ifndef FILENAMEEDITDELEGATE_H
#define FILENAMEEDITDELEGATE_H

#include <QStyledItemDelegate>

class FilenameEditDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    FilenameEditDelegate(QObject *parent = nullptr);

    int mIconWidth = 20;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // FILENAMEEDITDELEGATE_H
