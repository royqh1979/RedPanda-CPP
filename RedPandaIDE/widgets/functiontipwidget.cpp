#include "functiontipwidget.h"

#include <QHBoxLayout>

FunctionTipWidget::FunctionTipWidget(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::ToolTip);

    mLabel = new QLabel(this);
    mLabel->setText("Test");
    this->setLayout(new QHBoxLayout());
    this->layout()->addWidget(mLabel);
}
