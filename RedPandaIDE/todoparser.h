#ifndef TODOPARSER_H
#define TODOPARSER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QAbstractListModel>

struct TodoItem {
    QString filename;
    int lineNo;
    int ch;
    QString line;
};

using PTodoItem = std::shared_ptr<TodoItem>;

class TodoModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit TodoModel(QObject* parent=nullptr);
    void addItem(const QString& filename, int lineNo,
                 int ch, const QString& line);
    void clear();
    PTodoItem getItem(const QModelIndex& index);
private:
    QList<PTodoItem> mItems;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // QAbstractItemModel interface
public:
    int columnCount(const QModelIndex &parent) const override;
};

class TodoThread: public QThread
{
    Q_OBJECT
public:
    explicit TodoThread(const QString& filename, QObject* parent = nullptr);
signals:
    void parseStarted(const QString& filename);
    void todoFound(const QString& filename, int lineNo, int ch, const QString& line);
    void parseFinished();
private:
    QString mFilename;

    // QThread interface
protected:
    void run() override;
};

using PTodoThread = std::shared_ptr<TodoThread>;

class TodoParser : public QObject
{
    Q_OBJECT
public:
    explicit TodoParser(QObject *parent = nullptr);
    void parseFile(const QString& filename);
    bool parsing() const;

private:
    TodoThread* mThread;
    QRecursiveMutex mMutex;
};

using PTodoParser = std::shared_ptr<TodoParser>;

#endif // TODOPARSER_H
