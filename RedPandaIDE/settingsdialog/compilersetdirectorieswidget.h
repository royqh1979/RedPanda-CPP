#ifndef COMPILERSETDIRECTORIESWIDGET_H
#define COMPILERSETDIRECTORIESWIDGET_H

#include <QWidget>
#include <QStringListModel>

namespace Ui {
class CompilerSetDirectoriesWidget;
}

class QItemSelection;

class CompilerSetDirectoriesWidget : public QWidget
{
    Q_OBJECT
    class ListModel: public QStringListModel {
    public:
       Qt::ItemFlags flags(const QModelIndex &index) const;
    };

public:
    explicit CompilerSetDirectoriesWidget(QWidget *parent = nullptr);
    ~CompilerSetDirectoriesWidget();

    void setDirList(const QStringList& list);
    QStringList dirList() const;

private slots:
    void on_btnDelete_pressed();

    void on_btnAdd_pressed();

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void on_btnRemoveInvalid_pressed();

private:
    Ui::CompilerSetDirectoriesWidget *ui;
    ListModel* mModel;
};

#endif // COMPILERSETDIRECTORIESWIDGET_H
