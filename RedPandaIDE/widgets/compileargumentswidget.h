#ifndef COMPILEARGUMENTSWIDGET_H
#define COMPILEARGUMENTSWIDGET_H

#include <QMap>
#include <QWidget>
#include "../settings.h"

namespace Ui {
class CompileArgumentsWidget;
}

class CompileArgumentsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CompileArgumentsWidget(QWidget *parent = nullptr);
    ~CompileArgumentsWidget();

    QMap<QString, QString> options() const;

    void resetUI(Settings::PCompilerSet pSet, const QMap<QString,QString>& options);

private:
    Ui::CompileArgumentsWidget *ui;
    QString mCompilerType;
};

#endif // COMPILEARGUMENTSWIDGET_H
