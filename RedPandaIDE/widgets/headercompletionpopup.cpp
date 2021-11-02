#include "headercompletionpopup.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QVBoxLayout>

HeaderCompletionPopup::HeaderCompletionPopup(QWidget* parent):QWidget(parent)
{
    setWindowFlags(Qt::Popup);
    mListView = new CodeCompletionListView(this);
    mModel=new HeaderCompletionListModel(&mCompletionList);
    mListView->setModel(mModel);
    setLayout(new QVBoxLayout());
    layout()->addWidget(mListView);
    layout()->setMargin(0);

    mSearchLocal = false;
    mCurrentFile = "";
    mPhrase = "";
    mIgnoreCase = false;
}

HeaderCompletionPopup::~HeaderCompletionPopup()
{
    delete mModel;
}

void HeaderCompletionPopup::prepareSearch(const QString &phrase, const QString &fileName)
{
    QCursor oldCursor = cursor();
    setCursor(Qt::WaitCursor);
    mCurrentFile = fileName;
    mPhrase = phrase;
    getCompletionFor(phrase);
    setCursor(oldCursor);
}

bool HeaderCompletionPopup::search(const QString &phrase, bool autoHideOnSingleResult)
{
    mPhrase = phrase;
    if (mPhrase.isEmpty()) {
        hide();
        return false;
    }
    if(!isEnabled())
        return false;

    QCursor oldCursor = cursor();
    setCursor(Qt::WaitCursor);

    int i = mPhrase.lastIndexOf('\\');
    if (i<0) {
        i = mPhrase.lastIndexOf('/');
    }
    QString symbol = mPhrase;
    if (i>=0) {
        symbol = mPhrase.mid(i+1);
    }

    // filter fFullCompletionList to fCompletionList
    filterList(symbol);
    mModel->notifyUpdated();
    setCursor(oldCursor);

    if (!mCompletionList.isEmpty()) {
        mListView->setCurrentIndex(mModel->index(0,0));
        if (mCompletionList.count() == 1) {
            // if only one suggestion and auto hide , don't show the frame
            if (autoHideOnSingleResult)
                return true;
            // if only one suggestion, and is exactly the symbol to search, hide the frame (the search is over)
            if (symbol == mCompletionList.front())
                return true;
        }
    } else {
        hide();
    }
    return false;
}

void HeaderCompletionPopup::setKeypressedCallback(const KeyPressedCallback &newKeypressedCallback)
{
    mListView->setKeypressedCallback(newKeypressedCallback);
}

void HeaderCompletionPopup::setSuggestionColor(const QColor &color)
{
    mModel->setColor(color);
}

QString HeaderCompletionPopup::selectedFilename()
{
    if (!isEnabled())
        return "";
    int index = mListView->currentIndex().row();
    if (index>=0 && index<mCompletionList.count())
        return mCompletionList[index];
    else if (mCompletionList.count()>0) {
        return mCompletionList.front();
    }
        return "";
}

void HeaderCompletionPopup::filterList(const QString &member)
{
    mCompletionList.clear();
    if (member.isEmpty()) {
        foreach (const QString& s,mFullCompletionList) {
            mCompletionList.append(s);
        }
    } else {
        foreach (const QString& s,mFullCompletionList) {
            if (mIgnoreCase && s.startsWith(member, Qt::CaseInsensitive)) {
                mCompletionList.append(s);
            } else if (s.startsWith(member, Qt::CaseSensitive)){
                mCompletionList.append(s);
            }
        }
    }
    std::sort(mCompletionList.begin(),mCompletionList.end());
}

void HeaderCompletionPopup::getCompletionFor(const QString &phrase)
{
    int idx = phrase.lastIndexOf('\\');
    if (idx<0) {
        idx = phrase.lastIndexOf('/');
    }
    mFullCompletionList.clear();
    if (idx < 0) { // dont have basedir
        if (mSearchLocal) {
            QFileInfo fileInfo(mCurrentFile);
            addFilesInPath(fileInfo.absolutePath());
        };

        for (const QString& path: mParser->includePaths()) {
            addFilesInPath(path);
        }

        for (const QString& path: mParser->projectIncludePaths()) {
            addFilesInPath(path);
        }
    } else {
        QString current = phrase.mid(0,idx);
        if (mSearchLocal) {
            QFileInfo fileInfo(mCurrentFile);
            addFilesInSubDir(fileInfo.absolutePath(),current);
        }
        for (const QString& path: mParser->includePaths()) {
            addFilesInSubDir(path,current);
        }

        for (const QString& path: mParser->projectIncludePaths()) {
            addFilesInSubDir(path,current);
        }
    }
}

void HeaderCompletionPopup::addFilesInPath(const QString &path)
{
    QDir dir(path);
    if (!dir.exists())
        return;
    foreach (const QFileInfo& fileInfo, dir.entryInfoList()) {
        if (fileInfo.fileName().startsWith("."))
            continue;
        QString suffix = fileInfo.suffix().toLower();
        if (suffix == "h" || suffix == "hpp" || suffix == "") {
            addFile(fileInfo.fileName());
        }
    }
}

void HeaderCompletionPopup::addFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return;
    if (fileName.startsWith('.'))
        return;
    mFullCompletionList.insert(fileName);
}

void HeaderCompletionPopup::addFilesInSubDir(const QString &baseDirPath, const QString &subDirName)
{
    QDir baseDir(baseDirPath);
    QString subDirPath = baseDir.filePath(subDirName);
    addFilesInPath(subDirPath);
}

bool HeaderCompletionPopup::searchLocal() const
{
    return mSearchLocal;
}

void HeaderCompletionPopup::setSearchLocal(bool newSearchLocal)
{
    mSearchLocal = newSearchLocal;
}

bool HeaderCompletionPopup::ignoreCase() const
{
    return mIgnoreCase;
}

void HeaderCompletionPopup::setIgnoreCase(bool newIgnoreCase)
{
    mIgnoreCase = newIgnoreCase;
}

const QString &HeaderCompletionPopup::phrase() const
{
    return mPhrase;
}

void HeaderCompletionPopup::setParser(const PCppParser &newParser)
{
    mParser = newParser;
}

void HeaderCompletionPopup::showEvent(QShowEvent *)
{
    mListView->setFocus();
}

void HeaderCompletionPopup::hideEvent(QHideEvent *)
{
    mCompletionList.clear();
    mFullCompletionList.clear();
}

bool HeaderCompletionPopup::event(QEvent *event)
{
    bool result = QWidget::event(event);
    switch (event->type()) {
    case QEvent::FontChange:
        mListView->setFont(font());
        break;
    default:
        break;
    }
    return result;
}

HeaderCompletionListModel::HeaderCompletionListModel(const QStringList *files, QObject *parent):
    QAbstractListModel(parent),
    mFiles(files)
{

}

int HeaderCompletionListModel::rowCount(const QModelIndex &) const
{
    return mFiles->count();
}

QVariant HeaderCompletionListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()>=mFiles->count())
        return QVariant();

    switch(role) {
    case Qt::DisplayRole: {
        return mFiles->at(index.row());
        }
    case Qt::ForegroundRole:
        return mColor;
        break;
    }
    return QVariant();
}

void HeaderCompletionListModel::notifyUpdated()
{
    beginResetModel();
    endResetModel();
}

void HeaderCompletionListModel::setColor(const QColor &newColor)
{
    mColor = newColor;
}
