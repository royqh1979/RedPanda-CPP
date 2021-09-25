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
    mInfos.append(info);
}

void FunctionTooltipWidget::clearTips()
{
    mInfos.clear();
    hide();
}

int FunctionTooltipWidget::tipCount()
{
    return mInfos.count();
}

int FunctionTooltipWidget::paramPos() const
{
    return mParamPos;
}

void FunctionTooltipWidget::setParamPos(int newParamPos)
{
    mParamPos = newParamPos;
}

void FunctionTooltipWidget::nextTip()
{
    if (mInfos.length()>0)
        mInfoIndex = std::min(mInfoIndex+1,mInfos.length()-1);
    updateTip();
}

void FunctionTooltipWidget::previousTip()
{
    if (mInfos.length()>0)
        mInfoIndex = std::max(mInfoIndex-1,0);
    updateTip();
}

void FunctionTooltipWidget::updateTip()
{

    mTotalLabel->setVisible(mInfos.length()>1);
    mUpButton->setVisible(mInfos.length()>1);
    mDownButton->setVisible(mInfos.length()>1);
    mUpButton->setEnabled(mInfoIndex!=0);
    mDownButton->setEnabled(mInfoIndex!=mInfos.length()-1);
    if (mInfos.length()<=0)
        return;
    PFunctionInfo info = mInfos[mInfoIndex];
    QString text = info->returnType+ " " + info->name;
    if (info->params.length()==0) {
        text += "()";
    } else {
        QStringList displayList;
        for (int i=0;i<info->params.length();i++){
            const QString& param = info->params[i];
            if (mParamIndex == i) {
                displayList.append(QString("<b>%1</b>").arg(param));
            } else {
                displayList.append(param);
            }
        }
        text += "( "+displayList.join(", ") + ") ";
    }
    mInfoLabel->setText(text);
}

void FunctionTooltipWidget::guessFunction(int commas)
{
    for (int i=0;i<mInfos.size();i++) {
        if (mInfos[i]->params.count()>commas) {
            mInfoIndex = i;
            return;
        }
    }
    mInfoIndex = 0;
    return;
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

const QString &FunctionTooltipWidget::functionFullName() const
{
    return mFunctioFullName;
}

void FunctionTooltipWidget::setFunctioFullName(const QString &newFunctioFullName)
{
    mFunctioFullName = newFunctioFullName;
}

int FunctionTooltipWidget::paramIndex() const
{
    return mParamIndex;
}

void FunctionTooltipWidget::setParamIndex(int newParamIndex)
{
    mParamIndex = newParamIndex;
}

void FunctionTooltipWidget::closeEvent(QCloseEvent *)
{

}

void FunctionTooltipWidget::showEvent(QShowEvent *)
{
    if (mInfoIndex<0 || mInfoIndex>= mInfos.count()) {
        mInfoIndex = 0;
    }
    updateTip();
}

void FunctionTooltipWidget::hideEvent(QHideEvent *event)
{
    mInfos.clear();
}
