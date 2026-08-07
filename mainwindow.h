#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ImageWidget;
class DirModel;
class QSpinBox;
class QLabel;
class ImageMetaInfoModel;
class ThumbnailDelegate;
class QTimer;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void open(const QString& path);
private:
    void tryOpenSubDirInDirView(const QModelIndex &index);
private slots:
    void onCurrentDirChanged(const QString& oldPath, const QString& newPath);
    void onCurrentFileChanged(int oldFileId, int currentFileId);
    void onRequestPrevImage(bool scrollToBottom);
    void onRequestNextImage();
    void onDirViewCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void onZoomFactorChanged(int newVal);
    void onDirViewSizeChanged();
    void updateImageFitType();
    void slideShowNextImage();
    void onImageWidgetContextMenuRequested(const QPoint &pos);
    void onDirViewEnterPressed();
    void on_actionNext_triggered();

    void on_actionPrevious_triggered();

    void on_actionFirst_triggered();

    void on_actionLast_triggered();

    void on_actionOpen_triggered();

    void on_actionClose_triggered();
    void on_actionRotate_Left_triggered();

    void on_actionRotate_Right_triggered();

    void on_actionFlip_Horizontal_triggered();

    void on_actionFlip_Vertical_triggered();

    void on_actionStop_Animation_triggered();

    void on_actionPause_Animation_triggered();

    void on_actionNext_Frame_triggered();

    void on_actionPrev_Frame_triggered();

    void on_actionFull_Screen_triggered();

    void on_actionShow_Contents_toggled(bool arg1);

    void on_dockDir_visibilityChanged(bool visible);

    void on_actionOption_triggered();

    void on_actionImage_Meta_Info_toggled(bool arg1);

    void on_dockMetaInfo_visibilityChanged(bool visible);

    void on_actionExit_triggered();

    void on_actionPrint_triggered();

    void on_actionPrint_Preview_triggered();

    void on_actionCopy_triggered();

    void on_actionCopy_To_triggered();

    void on_actionRefresh_triggered();

    void on_actionSlide_Show_triggered();

    void on_actionAbout_triggered();

    void on_dirView_doubleClicked(const QModelIndex &index);

    void on_actionDelete_triggered();

    void onHistoryMenuAboutToShow();
    void onClearHistoryTriggered();

    void on_actionCopy_URL_triggered();

private:
    void updateStatusBar();
    void addToHistory(const QString &path);
    void applySettings();
    void updateActions();
private:
    Ui::MainWindow *ui;
    ImageWidget *mImageWidget;
    DirModel *mDirModel;
    QSpinBox *mZoomFactor;
    QLabel *mImageSizeInfo;
    QLabel *mPageInfo;
    QLabel *mImageNameInfo;
    ImageMetaInfoModel *mImageMetaInfoModel;
    ThumbnailDelegate *mThumbnailDelegate;
    bool mInFullScreen;
    bool mMaximizedBeforeFullScreen;
    QTimer *mSlideShowTimer;
    QList<QAction*> mHistoryActions;

    // QWidget interface
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
};
#endif // MAINWINDOW_H
