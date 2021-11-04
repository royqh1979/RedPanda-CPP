#ifndef OJPROBLEMPROPERTYWIDGET_H
#define OJPROBLEMPROPERTYWIDGET_H

#include <QDialog>

namespace Ui {
class OJProblemPropertyWidget;
}

class OJProblemPropertyWidget : public QDialog
{
    Q_OBJECT

public:
    explicit OJProblemPropertyWidget(QWidget *parent = nullptr);
    ~OJProblemPropertyWidget();
    void setName(const QString& name);
    void setUrl(const QString& url);
    void setDescription(const QString& description);
    QString name();
    QString url();
    QString description();

private slots:
    void on_btnOk_clicked();

    void on_btnCancel_clicked();

private:
    Ui::OJProblemPropertyWidget *ui;
};

#endif // OJPROBLEMPROPERTYWIDGET_H
