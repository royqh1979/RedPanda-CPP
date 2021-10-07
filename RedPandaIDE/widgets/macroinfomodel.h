#ifndef MACROINFOMODEL_H
#define MACROINFOMODEL_H

#include <QAbstractListModel>
#include <memory>

struct MacroInfo {
    QString macro;
    QString description;
};

using PMacroInfo = std::shared_ptr<MacroInfo>;

class MacroInfoModel: public QAbstractListModel{
    Q_OBJECT
public:
    explicit MacroInfoModel(QObject* parent = nullptr);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    PMacroInfo getInfo(const QModelIndex& index) const;
private:
    void addMacroInfo(const QString& macro, const QString& description);
private:
    QList<PMacroInfo> mMacroInfos;
};

#endif // MACROINFOMODEL_H
