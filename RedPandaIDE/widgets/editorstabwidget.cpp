#include "editorstabwidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include "../editor.h"
#include "../editorlist.h"
#include "../mainwindow.h"

EditorsTabWidget::EditorsTabWidget(QWidget* parent):QTabWidget(parent)
{
    setAcceptDrops(true);
}

void EditorsTabWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        foreach(const QUrl& url, event->mimeData()->urls()){
            if (!url.isLocalFile())
                continue;
            QString file = url.toLocalFile();
            if (getFileType(file)==FileType::Project) {
                pMainWindow->openProject(file);
                return;
            }
        }
        foreach(const QUrl& url, event->mimeData()->urls()){
            if (!url.isLocalFile())
                continue;
            QString file = url.toLocalFile();
            pMainWindow->openFile(file,this);
        }
    }
}

void EditorsTabWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()){
        event->acceptProposedAction();
    }
}
