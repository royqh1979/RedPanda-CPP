#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "imagewidget.h"
#include "dirmodel.h"
#include "imagemetainfomodel.h"
#include "thumbnailview.h"
#include "settings.h"
#include "settingsdialog/settingsdialog.h"
#include "aboutdialog.h"

#include <QHBoxLayout>
#include <QDebug>
#include <QFileDialog>
#include <QSpinBox>
#include <QLabel>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QScrollBar>
#include <QToolTip>
#include <QStyleFactory>
#include <QPrintDialog>
#include <QPrinter>
#include <QPainter>
#include <QPrintPreviewDialog>
#include <QClipboard>
#include <QMessageBox>
#include <QTimer>
#include <QDialog>
#include <QSpinBox>
#include <QPushButton>
#include <QActionGroup>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qApp->setStyle(QStyleFactory::create("fusion"));
    ui->setupUi(this);

    mSlideShowTimer = new QTimer{this};
    mSlideShowTimer->setSingleShot(true);
    mSlideShowTimer->setInterval(3000);
    connect(mSlideShowTimer, &QTimer::timeout,
            this, &MainWindow::slideShowNextImage);

    mInFullScreen = false;
    mImageNameInfo = new QLabel(this);
    ui->statusbar->addWidget(mImageNameInfo);
    mPageInfo = new QLabel(this);
    mPageInfo->setText(" ");
    ui->statusbar->addPermanentWidget(mPageInfo);
    mZoomFactor = new QSpinBox(this);
    mZoomFactor->setSuffix("%");
    mZoomFactor->setRange(1,1000);
    connect(mZoomFactor, qOverload<int>(&QSpinBox::valueChanged),
            this, &MainWindow::onZoomFactorChanged);
    ui->statusbar->addPermanentWidget(mZoomFactor);
    mImageSizeInfo = new QLabel(this);
    mImageSizeInfo->setText(" ");
    ui->statusbar->addPermanentWidget(mImageSizeInfo);

    mImageWidget = new ImageWidget(ui->centralwidget);
    QHBoxLayout *layout = new QHBoxLayout(ui->centralwidget);
#if QT_VERSION_MAJOR < 6
    layout->setMargin(0);
#endif
    layout->addWidget(mImageWidget);
    connect(mImageWidget, &ImageWidget::requestPrevImage,
            this, &MainWindow::onRequestPrevImage);
    connect(mImageWidget, &ImageWidget::requestNextImage,
            this, &MainWindow::onRequestNextImage);
    connect(mImageWidget, &ImageWidget::imageUpdated,
            this, &MainWindow::updateStatusBar);
    connect(mImageWidget, &ImageWidget::imageUpdated,
            this, &MainWindow::updateActions);
    connect(mImageWidget, &ImageWidget::playingChanged,
            this, &MainWindow::updateActions);
    connect(mImageWidget, &QWidget::customContextMenuRequested,
            this , &MainWindow::onImageWidgetContextMenuRequested);
    mImageWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    mDirModel = new DirModel(this);
    connect(mDirModel, &DirModel::currentFileChanged,
            this, &MainWindow::onCurrentFileChanged);
    connect(mDirModel, &DirModel::pathChanged,
            this, &MainWindow::onCurrentDirChanged);
    connect(mDirModel, &DirModel::pathChanged,
            this, &MainWindow::updateStatusBar);

    mThumbnailDelegate = new ThumbnailDelegate(this);
    ui->dirView->setItemDelegate(mThumbnailDelegate);
    ui->dirView->setModel(mDirModel);
    connect(ui->dirView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &MainWindow::onDirViewCurrentChanged);
    connect(ui->dirView, &ResizeawareListView::resized,
            this, &MainWindow::onDirViewSizeChanged);
    connect(ui->dirView, &ResizeawareListView::enterPressed,
            this, &MainWindow::onDirViewEnterPressed);
    ui->dirView->setAcceptDrops(false);
    mDirModel->setThumbnailSize(150);
    mThumbnailDelegate->setThumbnailSize(150);

    mImageMetaInfoModel = new ImageMetaInfoModel(this);
    ui->imageMetaInfoView->setModel(mImageMetaInfoModel);
    ui->imageMetaInfoView->setHeaderHidden(true);
    ui->dockMetaInfo->setVisible(false);
    ui->imageMetaInfoView->setAcceptDrops(false);

    QActionGroup *fitActionGroup = new QActionGroup(this);
    fitActionGroup->addAction(ui->actionFit_Width);
    fitActionGroup->addAction(ui->actionFit_Height);
    fitActionGroup->addAction(ui->actionFit_Page);
    fitActionGroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    //updateImageFitType();

    connect(ui->actionFit_Width, &QAction::triggered,
            this, &MainWindow::updateImageFitType);
    connect(ui->actionFit_Height, &QAction::triggered,
            this, &MainWindow::updateImageFitType);
    connect(ui->actionFit_Page, &QAction::triggered,
            this, &MainWindow::updateImageFitType);

    resize(pSettings->ui().mainWindowWidth(), pSettings->ui().mainWindowHeight());
    move(pSettings->ui().mainWindowLeft(), pSettings->ui().mainWindowTop());
    resizeDocks({ui->dockDir},{pSettings->ui().contentsPanelWidth()},Qt::Orientation::Horizontal);
    ui->dockDir->setVisible(pSettings->ui().showContentsPanel());
    ui->dirView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->dirView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    ui->menuAnimation->menuAction()->setVisible(false);

    setWindowIcon(QPixmap(":/icons/imageviewer.png"));
    setAcceptDrops(true);
    updateActions();
    updateStatusBar();
    applySettings();

    connect(ui->menuHistory, &QMenu::aboutToShow,
            this, &MainWindow::onHistoryMenuAboutToShow);
    connect(ui->actionClearHistory, &QAction::triggered,
            this, &MainWindow::onClearHistoryTriggered);

    addActions(menuBar()->actions());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::open(const QString &path)
{
    mSlideShowTimer->stop();
    ui->statusbar->showMessage(tr("Openning \"%1\"").arg(path));
    mDirModel->open(path);
    if (mDirModel->imageCount() - mDirModel->subDirCount() >0)
        addToHistory(mDirModel->path());
    ui->statusbar->clearMessage();
}

void MainWindow::addToHistory(const QString &path)
{
    if (path.isEmpty())
        return;
    QFileInfo info(path);
    QString dirPath = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    pSettings->history().addRecentDir(dirPath);
}

void MainWindow::onHistoryMenuAboutToShow()
{
    for (QAction *action : mHistoryActions) {
        ui->menuHistory->removeAction(action);
        delete action;
    }
    mHistoryActions.clear();

    QStringList recentDirs = pSettings->history().recentDirs();
    if (recentDirs.isEmpty()) {
        QAction *placeholder = new QAction(tr("(No recent folders)"), ui->menuHistory);
        placeholder->setEnabled(false);
        ui->menuHistory->insertAction(ui->actionClearHistory, placeholder);
        mHistoryActions.append(placeholder);
        return;
    }

    for (const QString &dir : recentDirs) {
        QAction *action = new QAction(dir, ui->menuHistory);
        connect(action, &QAction::triggered, this, [this, dir]() {
            open(dir);
        });
        ui->menuHistory->insertAction(ui->actionClearHistory, action);
        mHistoryActions.append(action);
    }
}

void MainWindow::onClearHistoryTriggered()
{
    pSettings->history().clearRecentDirs();
    for (QAction *action : mHistoryActions) {
        ui->menuHistory->removeAction(action);
        delete action;
    }
    mHistoryActions.clear();
}

void MainWindow::tryOpenSubDirInDirView(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    QString path = mDirModel->imagePath(index.row());
    QFileInfo info{path};
    if (info.isDir()) {
        open(path);
    }
}

void MainWindow::onCurrentDirChanged(const QString &oldPath, const QString &newPath)
{
    QFileInfo dir{newPath};
    if (dir.isDir()) {
        setWindowTitle(tr("QImageView [%1]").arg(dir.absoluteFilePath()));
    } else {
        setWindowTitle(tr("QImageView").arg(dir.absoluteFilePath()));
    }
}

void MainWindow::onCurrentFileChanged(int oldFileId, int currentFileId)
{
    if (mDirModel->imageCount()>0) {
        QModelIndex index = mDirModel->index(currentFileId,0);

        ui->dirView->selectionModel()->select(index,QItemSelectionModel::SelectionFlag::Select);
        ui->dirView->setCurrentIndex(index);
        mImageWidget->setImage(mDirModel->imagePath(mDirModel->currentFileIdx()));
        mImageMetaInfoModel->setImagePath(mDirModel->imagePath(mDirModel->currentFileIdx()));
        ui->imageMetaInfoView->setFirstColumnSpanned(0,QModelIndex(),true);
        ui->imageMetaInfoView->setFirstColumnSpanned(1,QModelIndex(),true);
        ui->imageMetaInfoView->setFirstColumnSpanned(2,QModelIndex(),true);
        ui->imageMetaInfoView->expandAll();
        ui->imageMetaInfoView->resizeColumnToContents(0);
    } else {
        mImageWidget->setImage("");
        mImageMetaInfoModel->setImagePath("");
    }
    updateActions();
    updateStatusBar();
}

void MainWindow::onRequestPrevImage(bool scrollToBottom)
{
    mDirModel->toPrevious();
    if (scrollToBottom) {
        mImageWidget->scrollToBottom();
    }
}

void MainWindow::onRequestNextImage()
{
    mDirModel->toNext();
}

void MainWindow::onDirViewCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (!current.isValid())
        return;
    mDirModel->setCurrentFileIdx(current.row());
}

void MainWindow::onZoomFactorChanged(int newVal)
{
    mImageWidget->setRatio(newVal/100.0);
}

void MainWindow::onDirViewSizeChanged()
{
    pSettings->ui().setContentsPanelWidth(ui->dockDir->width());
    int width = ui->dirView->width()-ui->dirView->verticalScrollBar()->sizeHint().width()-20;
    mThumbnailDelegate->setThumbnailSize(width);
    mDirModel->setThumbnailSize(width);
    ui->dirView->doItemsLayout();
    ui->dirView->scrollTo(ui->dirView->currentIndex());
}

void MainWindow::updateImageFitType()
{
    if (ui->actionFit_Height->isChecked())
        mImageWidget->setFitType(ImageWidget::AutoFitType::Height);
    else if (ui->actionFit_Width->isChecked())
        mImageWidget->setFitType(ImageWidget::AutoFitType::Width);
    else if (ui->actionFit_Page->isChecked())
        mImageWidget->setFitType(ImageWidget::AutoFitType::Page);
    else
        mImageWidget->setFitType(ImageWidget::AutoFitType::None);
}

void MainWindow::slideShowNextImage()
{
    mSlideShowTimer->start();
    if (mDirModel->currentFileIdx() == mDirModel->imageCount()-1)
        mDirModel->toFirst();
    else
        mDirModel->toNext();
}

void MainWindow::onImageWidgetContextMenuRequested(const QPoint &pos)
{
    QMenu *menu=new QMenu(this);
    menu->addAction(ui->actionFull_Screen);
    menu->addSeparator();
    menu->addAction(ui->actionSlide_Show);
    menu->addSeparator();
    menu->addAction(ui->actionFit_Page);
    menu->addAction(ui->actionFit_Width);
    menu->addAction(ui->actionFit_Height);
    menu->popup(mImageWidget->mapToGlobal(pos));
}

void MainWindow::onDirViewEnterPressed()
{
    tryOpenSubDirInDirView(ui->dirView->currentIndex());
}

void MainWindow::on_actionNext_triggered()
{
    mDirModel->toNext();
}


void MainWindow::on_actionPrevious_triggered()
{
    mDirModel->toPrevious();
}


void MainWindow::on_actionFirst_triggered()
{
    mDirModel->toFirst();
}


void MainWindow::on_actionLast_triggered()
{
    mDirModel->toLast();
}


void MainWindow::on_actionOpen_triggered()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Choose image file"));
    if (!path.isEmpty())
        open(path);
}


void MainWindow::on_actionClose_triggered()
{
    mSlideShowTimer->stop();
    mDirModel->close();
}

void MainWindow::updateStatusBar()
{
    if (mDirModel->imageCount()<=0 || mDirModel->currentFileIdx()==-1) {
        mZoomFactor->blockSignals(true);
        mZoomFactor->setValue(100);
        mZoomFactor->blockSignals(false);
        mZoomFactor->setEnabled(false);
        mImageSizeInfo->setText(" ");
        mPageInfo->setText(" ");
        mImageNameInfo->setText("");
    } else {
        mZoomFactor->blockSignals(true);
        mZoomFactor->setValue(mImageWidget->ratio()*100);
        mZoomFactor->blockSignals(false);
        mZoomFactor->setEnabled(true);
        mImageSizeInfo->setText(QString(" %1x%2 ").arg(mImageWidget->imageSize().width()).arg(mImageWidget->imageSize().height()));
        mPageInfo->setText(QString("%1/%2").arg(mDirModel->currentFileIdx()+1)
                               .arg(mDirModel->imageCount()));
        QString imageName = mDirModel->imageFileName(mDirModel->currentFileIdx());
        QString s;
        if (mImageWidget->isAnimation()) {
            s=QString("%1[%2/%3]")
                    .arg(imageName)
                    .arg(mImageWidget->currentFrameNumber()+1)
                    .arg(mImageWidget->frameCount());
        } else {
            s=imageName;
        }
        mImageNameInfo->setText(s);
    }
}

void MainWindow::applySettings()
{
    QFont font{pSettings->ui().fontName(), pSettings->ui().fontSize()};
    qApp->setFont(font);
    setFont(font);

    ui->actionFit_Width->setChecked(pSettings->view().fitMode() == "Width");
    ui->actionFit_Height->setChecked(pSettings->view().fitMode() == "Height");
    ui->actionFit_Page->setChecked(pSettings->view().fitMode() == "Page");

    updateImageFitType();
}

void MainWindow::updateActions()
{
    bool hasImage = (mDirModel->imageCount()>0);
    ui->actionClose->setEnabled(hasImage);
    ui->actionCopy_To->setEnabled(hasImage);
    ui->actionPrint->setEnabled(hasImage);
    ui->actionPrint_Preview->setEnabled(hasImage);
    ui->actionSlide_Show->setEnabled(mDirModel->imageCount()>1);
    ui->actionRefresh->setEnabled(!mDirModel->path().isEmpty());
    ui->actionFirst->setEnabled(hasImage);
    ui->actionLast->setEnabled(hasImage);
    ui->actionPrevious->setEnabled(hasImage);
    ui->actionNext->setEnabled(hasImage);
    ui->actionRotate_Left->setEnabled(hasImage);
    ui->actionRotate_Right->setEnabled(hasImage);
    ui->actionFlip_Horizontal->setEnabled(hasImage);
    ui->actionFlip_Vertical->setEnabled(hasImage);
    ui->actionCopy->setEnabled(hasImage);
    bool isAnimation = mImageWidget->isAnimation();
    ui->menuAnimation->menuAction()->setVisible(isAnimation);
    ui->menuAnimation->menuAction()->setEnabled(isAnimation);
    bool isPlayingAnimation = mImageWidget->playing();
    bool canPlayingAnimation = mImageWidget->canPlay();
    ui->actionPause_Animation->setEnabled(canPlayingAnimation);
    ui->actionStop_Animation->setEnabled(canPlayingAnimation && mImageWidget->currentFrameNumber()!=0);
    ui->actionNext_Frame->setEnabled(isAnimation && !isPlayingAnimation);
    ui->actionPrev_Frame->setEnabled(isAnimation && !isPlayingAnimation);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->urls().count()==1) {
        QList<QUrl> urlList = mimeData->urls();
        QString fileName = urlList.first().toLocalFile();
        if (QFileInfo::exists(fileName))
            event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->urls().count()==1) {
        QList<QUrl> urlList = mimeData->urls();
        QString fileName = urlList.first().toLocalFile();
        open(fileName);
        event->acceptProposedAction();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier
            && mInFullScreen) {
        on_actionFull_Screen_triggered();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    pSettings->ui().setShowContentsPanel(ui->actionShow_Contents->isChecked());
    pSettings->ui().setMainWindowWidth(width());
    pSettings->ui().setMainWindowHeight(height());
    pSettings->ui().setMainWindowLeft(pos().x());
    pSettings->ui().setMainWindowTop(pos().y());
    if (ui->actionShow_Contents->isChecked())
        pSettings->ui().setContentsPanelWidth(ui->dockDir->width());
    if (ui->actionFit_Width->isChecked())
        pSettings->view().setFitMode("Width");
    else if (ui->actionFit_Height->isChecked())
        pSettings->view().setFitMode("Height");
    else if (ui->actionFit_Page->isChecked())
        pSettings->view().setFitMode("Page");
    else
        pSettings->view().setFitMode("None");
}

void MainWindow::on_actionRotate_Left_triggered()
{
    mImageWidget->rotate(-90);
}


void MainWindow::on_actionRotate_Right_triggered()
{
    mImageWidget->rotate(90);
}


void MainWindow::on_actionFlip_Horizontal_triggered()
{
    mImageWidget->horizontalFlip();
}


void MainWindow::on_actionFlip_Vertical_triggered()
{
    mImageWidget->verticalFlip();
}


void MainWindow::on_actionStop_Animation_triggered()
{
    mImageWidget->stop();
}


void MainWindow::on_actionPause_Animation_triggered()
{
    mImageWidget->pause();
}


void MainWindow::on_actionNext_Frame_triggered()
{
    mImageWidget->nextFrame();
}


void MainWindow::on_actionPrev_Frame_triggered()
{
    mImageWidget->prevFrame();
}


void MainWindow::on_actionFull_Screen_triggered()
{
    if (mInFullScreen) {
        ui->dockDir->blockSignals(true);
        if (ui->dockDir->isVisible()!=ui->actionShow_Contents->isChecked())
            ui->dockDir->setVisible(ui->actionShow_Contents->isChecked());
        ui->dockDir->blockSignals(false);
        ui->toolBar->setVisible(true);
        ui->menubar->setVisible(true);
        ui->statusbar->setVisible(true);
        if (mMaximizedBeforeFullScreen)
            showMaximized();
        else
            showNormal();
        mInFullScreen = false;
    } else {
        mMaximizedBeforeFullScreen = isMaximized();
        ui->dockDir->blockSignals(true);
        ui->dockDir->setVisible(false);
        ui->dockDir->blockSignals(false);
        ui->toolBar->setVisible(false);
        ui->menubar->setVisible(false);
        ui->statusbar->setVisible(false);
        showFullScreen();
        mInFullScreen = true;
    }
}


void MainWindow::on_actionShow_Contents_toggled(bool arg1)
{
    Q_UNUSED(arg1);
    if (!mInFullScreen)
        ui->dockDir->setVisible(ui->actionShow_Contents->isChecked());
}


void MainWindow::on_dockDir_visibilityChanged(bool visible)
{
    ui->actionShow_Contents->blockSignals(true);
    ui->actionShow_Contents->setChecked(visible);
    ui->actionShow_Contents->blockSignals(false);
}


void MainWindow::on_actionOption_triggered()
{
    PSettingsDialog optionDlg = SettingsDialog::optionDialog(this);
    connect(optionDlg.get(), &SettingsDialog::settingsChanged,
            this, &MainWindow::applySettings);
    optionDlg->exec();
}


void MainWindow::on_actionImage_Meta_Info_toggled(bool arg1)
{
    if (!mInFullScreen)
        ui->dockMetaInfo->setVisible(ui->actionImage_Meta_Info->isChecked());
}


void MainWindow::on_dockMetaInfo_visibilityChanged(bool visible)
{
    ui->actionImage_Meta_Info->blockSignals(true);
    ui->actionImage_Meta_Info->setChecked(visible);
    ui->actionImage_Meta_Info->blockSignals(false);
}


void MainWindow::on_actionExit_triggered()
{
    close();
}


void MainWindow::on_actionPrint_triggered()
{
    QPrinter printer(QPrinter::HighResolution);

    // 弹出打印对话框，用户可选择打印机、份数、方向等
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QPainter painter(&printer);

    QPixmap pixmap = QPixmap::fromImage(mImageWidget->currentFrame());
    // 计算缩放比例，让图片完整打印在页面内（保持宽高比）
    QRect rect = painter.viewport();
    QSize size = pixmap.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);

    QRect targetRect(QPoint(0, 0), size);
    targetRect.moveCenter(rect.center());

    painter.drawPixmap(targetRect, pixmap);
    painter.end();
}


void MainWindow::on_actionPrint_Preview_triggered()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);

    QPixmap pixmap = QPixmap::fromImage(mImageWidget->currentFrame());

    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
                     [&pixmap](QPrinter *printer) {
        QPainter painter(printer);
        QRect rect = painter.viewport();
        QSize size = pixmap.size();
        size.scale(rect.size(), Qt::KeepAspectRatio);

        QRect targetRect(QPoint(0, 0), size);
        targetRect.moveCenter(rect.center());

        painter.drawPixmap(targetRect, pixmap);
    });

    preview.exec();
}


void MainWindow::on_actionCopy_triggered()
{
    QPixmap pixmap = QPixmap::fromImage(mImageWidget->currentFrame());
    if (!pixmap.isNull()) {
        QMimeData * mimeData = new QMimeData;
        QList<QUrl> urls;
        urls.append(QUrl::fromLocalFile(mImageWidget->imagePath()));
        mimeData->setImageData(pixmap);
        mimeData->setUrls(urls);
        QGuiApplication::clipboard()->clear();
        QGuiApplication::clipboard()->setMimeData(mimeData);
    }
}


void MainWindow::on_actionCopy_To_triggered()
{
    if (!QFileInfo::exists(mImageWidget->imagePath()))
        return;
    QString newPath = QFileDialog::getExistingDirectory(this, tr("Copy Image File"));
    if (newPath.isEmpty())
        return;
    newPath = QDir{newPath}.absoluteFilePath(QFileInfo{mImageWidget->imagePath()}.fileName());
    bool isOk = QFile::copy(mImageWidget->imagePath(),newPath);
    if (isOk)
        QMessageBox::information(this,tr("Copy Succeed"),tr("Image has been successfully copied!"));
    else
        QMessageBox::critical(this,tr("Copy Failed"),tr("Image copy failed!"));
}


void MainWindow::on_actionRefresh_triggered()
{
    mDirModel->open(mImageWidget->imagePath());
}


void MainWindow::on_actionSlide_Show_triggered()
{
    if (mSlideShowTimer->isActive())
        mSlideShowTimer->stop();
    else {
        if (mDirModel->imageCount()<=1)
            return;
        QDialog dlg;
        dlg.setWindowTitle(tr("Slide Show"));
        dlg.resize(300,150);
        QLabel *label = new QLabel("Delay Time(ms)", &dlg);
        QSpinBox *edit = new QSpinBox(&dlg);
        edit->setMinimum(1);
        edit->setMaximum(999999);
        edit->setValue(pSettings->view().slideShowDelayTime());

        QFrame *inputFrame = new QFrame{&dlg};
        inputFrame->setFrameShape(QFrame::NoFrame);
        inputFrame->setFrameShadow(QFrame::Plain);

        QHBoxLayout *inputLayout = new QHBoxLayout{inputFrame};
        inputLayout->addWidget(label);
        inputLayout->addWidget(edit);
        inputFrame->setLayout(inputLayout);

        QFrame *btnFrame = new QFrame{&dlg};
        btnFrame->setFrameShape(QFrame::NoFrame);
        btnFrame->setFrameShadow(QFrame::Plain);
        QPushButton *okBtn = new QPushButton(tr("Ok"), btnFrame);
        QPushButton *cancelBtn = new QPushButton(tr("Cancel"), btnFrame);

        QHBoxLayout *btnLayout = new QHBoxLayout{&dlg};
        btnLayout->addStretch(1);
        btnLayout->addWidget(okBtn);
        btnLayout->addWidget(cancelBtn);
        btnFrame->setLayout(btnLayout);

        QVBoxLayout *mainLayout = new QVBoxLayout{&dlg};  // 直接设置给 dialog
        mainLayout->addWidget(inputFrame);
        mainLayout->addStretch(1);
        mainLayout->addWidget(btnFrame);
        dlg.setLayout(mainLayout);

        connect(okBtn, &QPushButton::clicked, [this, &dlg, edit]() {
            pSettings->view().setSlideShowDelayTime(edit->value());
            mSlideShowTimer->setInterval(edit->value());
            mSlideShowTimer->start();
            dlg.accept();
        });

        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

        dlg.exec();
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dlg{this};
    dlg.exec();
}


void MainWindow::on_dirView_doubleClicked(const QModelIndex &index)
{
    tryOpenSubDirInDirView(index);
}


void MainWindow::on_actionDelete_triggered()
{
    if (QFileInfo::exists(mImageWidget->imagePath())) {
        if (QMessageBox::warning(this, tr("Deleting File"), tr("File '%1' will be deleted, are you sure?").arg(mImageWidget->imagePath()),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No) == QMessageBox::Yes) {
            if (QFile::remove(mImageWidget->imagePath())){
                on_actionPrevious_triggered();
                on_actionRefresh_triggered();
            } else {
                QMessageBox::critical(this,tr("Deletion failed"),tr("Failed to delete file '%1'!").arg(mImageWidget->imagePath()));
            }
        }
    }
}

