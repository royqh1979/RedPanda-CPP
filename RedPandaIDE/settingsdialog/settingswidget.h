#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>

class QAbstractItemView;
class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(const QString& name, const QString& group, QWidget *parent = nullptr);

    virtual void init();

    void load();
    void save();
signals:
    void settingsChanged(bool changed);

protected:
    virtual void doLoad() = 0;
    virtual void doSave() = 0;
    void connectAbstractItemView(QAbstractItemView* pView);
    void disconnectAbstractItemView(QAbstractItemView* pView);
public:
    const QString& group();
    const QString& name();
    bool isSettingsChanged();
    void connectInputs();
    void disconnectInputs();
public slots:
    void setSettingsChanged();
    void clearSettingsChanged();
private:

private:
    bool mSettingsChanged;
    QString mGroup;
    QString mName;
};

#endif // SETTINGSWIDGET_H
