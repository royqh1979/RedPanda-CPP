#ifndef COMPILEARGUMENTSWIDGET_H
#define COMPILEARGUMENTSWIDGET_H

#include <QMap>
#include <QTabWidget>
#include "../settings.h"

namespace Ui {
class CompileArgumentsWidget;
}

class CompileArgumentsWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit CompileArgumentsWidget(QWidget *parent = nullptr);
    ~CompileArgumentsWidget();

    QMap<QString, QString> arguments(bool includeUnset) const;

    void resetUI(Settings::PCompilerSet pSet, const QMap<QString,QString>& options);

private:
    QString mCompilerType;
};

#endif // COMPILEARGUMENTSWIDGET_H
