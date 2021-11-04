#include "ojproblempropertywidget.h"
#include "ui_ojproblempropertywidget.h"

OJProblemPropertyWidget::OJProblemPropertyWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OJProblemPropertyWidget)
{
    ui->setupUi(this);
}

OJProblemPropertyWidget::~OJProblemPropertyWidget()
{
    delete ui;
}

void OJProblemPropertyWidget::setName(const QString &name)
{
    QFont f = ui->lbName->font();
    f.setPointSize(f.pointSize()+2);
    f.setBold(true);
    ui->lbName->setFont(f);
    ui->lbName->setText(name);
}

void OJProblemPropertyWidget::setUrl(const QString &url)
{
    ui->txtURL->setText(url);
}

void OJProblemPropertyWidget::setDescription(const QString &description)
{
    ui->txtDescription->setHtml(description);
}

QString OJProblemPropertyWidget::name()
{
    return ui->lbName->text();
}

QString OJProblemPropertyWidget::url()
{
    return ui->txtURL->text();
}

QString OJProblemPropertyWidget::description()
{
    return ui->txtDescription->toHtml();
}

void OJProblemPropertyWidget::on_btnOk_clicked()
{
    this->accept();
}


void OJProblemPropertyWidget::on_btnCancel_clicked()
{
    this->reject();
}

