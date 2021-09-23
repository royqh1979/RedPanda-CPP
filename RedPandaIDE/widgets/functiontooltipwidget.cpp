#include "functiontooltipwidget.h"

#include <QHBoxLayout>

FunctionTooltipWidget::FunctionTooltipWidget(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::Popup);
    setFocusPolicy(Qt::NoFocus);
    mInfoLabel = new QLabel(this);
    mInfoLabel->setWordWrap(true);
    mInfoLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    mTotalLabel = new QLabel(this);
    mTotalLabel->setWordWrap(false);
    mTotalLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
    mUpButton = new QToolButton(this);
    mUpButton->setArrowType(Qt::UpArrow);
    mUpButton->setFixedSize(16, 16);
    mUpButton->setAutoRaise(true);
    mDownButton = new QToolButton(this);
    mDownButton->setArrowType(Qt::DownArrow);
    mDownButton->setFixedSize(16, 16);
    mDownButton->setAutoRaise(true);


    this->setLayout(new QHBoxLayout());
    layout()->addWidget(mUpButton);
    layout()->addWidget(mTotalLabel);
    layout()->addWidget(mDownButton);
    layout()->addWidget(mInfoLabel);
}

void FunctionTooltipWidget::addTip(const QString &name, const QString& fullname,
                                   const QString &returnType, const QString &args, const QString &noNameArgs)
{
    PFunctionInfo info = std::make_shared<FunctionInfo>();
    info->name = name;
    info->fullname = fullname;
    info->returnType = returnType;
    info->params = splitArgs(args);
    info->nonameParams = splitArgs(noNameArgs);
}

void FunctionTooltipWidget::clearTips()
{
    mInfos.clear();
    hide();
}

int FunctionTooltipWidget::paramPos() const
{
    return mParamPos;
}

void FunctionTooltipWidget::setParamPos(int newParamPos)
{
    mParamPos = newParamPos;
}

int FunctionTooltipWidget::index() const
{
    return mIndex;
}

void FunctionTooltipWidget::setIndex(int newIndex)
{
    mIndex = newIndex;
}

QStringList FunctionTooltipWidget::splitArgs(QString argStr)
{
    int i = 0;
    // Split up argument string by ,
    if (argStr.startsWith('(')) {
        i = 1; // assume it starts with ( and ends with )
    } else {
        i = 0;
    }
    int paramStart = i;

    QStringList result;
    while (i < argStr.length()) {
        if ((argStr[i] == ',')) {
            // We've found "int* a" for example
            QString s = argStr.mid(paramStart,i-paramStart);
            result.append(s);
            paramStart = i + 1; // step over ,
        }
        i++;
    }
    QString s = argStr.mid(paramStart,i-paramStart);
    s=s.trimmed();
    if (!s.isEmpty()) {
        result.append(s);
    }
    return result;
}
