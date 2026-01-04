/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "functiontooltipwidget.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <qt_utils/utils.h>

FunctionTooltipWidget::FunctionTooltipWidget(QWidget *parent) :
    QFrame{parent, Qt::ToolTip | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus},
    mMinWidth{410}
{
    setFocusPolicy(Qt::NoFocus);
    mInfoLabel = new QLabel(this);
    mInfoLabel->setWordWrap(true);
    mInfoLabel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    mInfoLabel->setTextFormat(Qt::TextFormat::RichText);
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
    mInfoIndex=0;

    this->setLayout(new QHBoxLayout());
    layout()->setContentsMargins(0,0,0,0);
    layout()->setSpacing(0);
    layout()->addWidget(mUpButton);
    layout()->addWidget(mTotalLabel);
    layout()->addWidget(mDownButton);
    layout()->addWidget(mInfoLabel);
    QSizePolicy policy=mInfoLabel->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    mInfoLabel->setSizePolicy(policy);
    connect(mUpButton,&QPushButton::clicked,
            this,&FunctionTooltipWidget::previousTip);
    connect(mDownButton,&QPushButton::clicked,
            this,&FunctionTooltipWidget::nextTip);
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
    mInfoIndex=0;
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
    if (mInfoIndex>=mInfos.length()-1) {
        hide();
        return;
    }
    if (mInfos.length()>0)
        mInfoIndex = std::min(mInfoIndex+1,mInfos.length()-1);
    updateTip();
}

void FunctionTooltipWidget::previousTip()
{
    if (mInfoIndex==0) {
        hide();
        return ;
    }
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
    QString originText = text;
    if (info->params.length()==0) {
        text += "()";
        originText += "()";
    } else {
        QStringList displayList;
        QStringList originList;
        for (int i=0;i<info->params.length();i++){
            QString param = info->params[i];
            originList.append(param);

            param.replace("<","&lt;");
            param.replace(">","&gt;");
            if (mParamIndex == i) {
                displayList.append(QString("<b>%1</b>").arg(param));
            } else {
                displayList.append(param);
            }
        }
        text += "( "+displayList.join(", ") + ") ";
        originText += "( "+originList.join(", ") + ") ";
    }
    if (mInfos.length()>1) {
        mTotalLabel->setText(QString("%1/%2").arg(mInfoIndex+1).arg(mInfos.length()));
    }
    int width = mInfoLabel->fontMetrics().horizontalAdvance(originText)+10;
    if (width > mMinWidth) {
        mInfoLabel->setMinimumWidth(mMinWidth);
    } else {
        mInfoLabel->setMinimumWidth(width);
    }
    mInfoLabel->setText(text);
}

void FunctionTooltipWidget::guessFunction(int commas)
{
    if (mInfoIndex>=0 && mInfoIndex<mInfos.count()
            && mInfos[mInfoIndex]->params.count()>commas)
        return;
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
        if (s.endsWith(')'))
            s.truncate(s.length()-1);
        result.append(s);
    }
    return result;
}

int FunctionTooltipWidget::minWidth() const
{
    return mMinWidth;
}

void FunctionTooltipWidget::setMinWidth(int newMinWidth)
{
    mMinWidth = newMinWidth;
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
    updateTip();
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

void FunctionTooltipWidget::hideEvent(QHideEvent *)
{
    mInfos.clear();
    mFunctioFullName = "";
}
