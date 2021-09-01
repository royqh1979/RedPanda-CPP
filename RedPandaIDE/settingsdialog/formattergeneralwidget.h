#ifndef FORMATTERGENERALWIDGET_H
#define FORMATTERGENERALWIDGET_H

#include <QAbstractListModel>
#include <QWidget>
#include "settingswidget.h"
#include "../utils.h"
#include "../settings.h"

namespace Ui {
class FormatterGeneralWidget;
}

struct FormatterStyleItem {
    QString name;
    QString description;
    FormatterBraceStyle style;
    explicit FormatterStyleItem(const QString& name,
                                const QString& description,
                                FormatterBraceStyle style);
};
using PFormatterStyleItem = std::shared_ptr<FormatterStyleItem>;

class FormatterStyleModel : public QAbstractListModel{
    Q_OBJECT
    // QAbstractItemModel interface
public:
    explicit FormatterStyleModel(QObject *parent=nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    PFormatterStyleItem getStyle(const QModelIndex &index);
    PFormatterStyleItem getStyle(int index);
private:
    QList<PFormatterStyleItem> mStyles;
};


class FormatterGeneralWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit FormatterGeneralWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~FormatterGeneralWidget();
private slots:
    void onBraceStyleChanged();

    void on_chkBreakMaxCodeLength_stateChanged(int arg1);

    void updateDemo();
private:
    void updateCodeFormatter(Settings::CodeFormatter& format);

private:
    Ui::FormatterGeneralWidget *ui;
    FormatterStyleModel mStylesModel;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // FORMATTERGENERALWIDGET_H
