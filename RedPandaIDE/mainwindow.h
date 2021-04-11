#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class EditorList;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateStatusBarForEncoding();


private slots:
    void on_actionNew_triggered();

    void on_EditorTabsLeft_tabCloseRequested(int index);

    void on_actionOpen_triggered();

    void on_actionSave_triggered();

    void on_actionSaveAs_triggered();

private:
    void setupActions();

private:
    Ui::MainWindow *ui;
    EditorList* mEditorList;
    QLabel* mFileInfoStatus;
    QLabel* mFileEncodingStatus;

   // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

extern MainWindow* pMainWindow;
#endif // MAINWINDOW_H
