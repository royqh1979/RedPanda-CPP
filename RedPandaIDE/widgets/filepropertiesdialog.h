#ifndef FILEPROPERTIESDIALOG_H
#define FILEPROPERTIESDIALOG_H

#include <QAbstractListModel>
#include <QDialog>

namespace Ui {
class FilePropertiesDialog;
}

class FilePropertiesModel: public QAbstractListModel {
    Q_OBJECT
public:
    explicit FilePropertiesModel(QObject* parent=nullptr);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};
class Editor;
class FilePropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilePropertiesDialog(Editor* activeEditor,QWidget *parent = nullptr);
    ~FilePropertiesDialog();

private:
    void calcFile(Editor* editor,
                  int& totalLines,
                  int &commentLines,
                  int &emptyLines,
                  int &codeLines,
                  int &includeLines);
private:
    FilePropertiesModel mModel;
    Editor * mActiveEditor;
private:
    Ui::FilePropertiesDialog *ui;

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
private slots:
    void on_cbFiles_currentIndexChanged(int index);
    void on_btnOK_clicked();
};

#endif // FILEPROPERTIESDIALOG_H
