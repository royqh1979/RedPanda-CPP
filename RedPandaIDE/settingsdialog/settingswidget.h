#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>

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
    void connectInputs();
    virtual void doLoad() = 0;
    virtual void doSave() = 0;
public:
    const QString& group();
    const QString& name();
    bool isSettingsChanged();
public slots:
    void setSettingsChanged();
    void clearSettingsChanged();
private:
    bool mSettingsChanged;
    QString mGroup;
    QString mName;
};

#endif // SETTINGSWIDGET_H
