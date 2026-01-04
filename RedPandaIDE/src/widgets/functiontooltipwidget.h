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
#ifndef FUNCTIONTOOLTIPWIDGET_H
#define FUNCTIONTOOLTIPWIDGET_H
#include <QLabel>
#include <QToolButton>
#include <QWidget>
#include <memory>

struct FunctionInfo {
    QString name;
    QString fullname;
    QStringList params;
    QStringList nonameParams;
    QString returnType;
};

using PFunctionInfo = std::shared_ptr<FunctionInfo>;

class FunctionTooltipWidget : public QFrame
{
    Q_OBJECT
public:
    explicit FunctionTooltipWidget(QWidget *parent = nullptr);
    void addTip(const QString& name, const QString& fullName,
                const QString& returnType, const QString& args,
                const QString& noNameArgs);
    void clearTips();
    int tipCount();
    int paramPos() const;
    void setParamPos(int newParamPos);
    void nextTip();
    void previousTip();
    void updateTip();
    void guessFunction(int commas);

    int paramIndex() const;
    void setParamIndex(int newParamIndex);

    const QString &functionFullName() const;
    void setFunctioFullName(const QString &newFunctioFullName);

    int minWidth() const;
    void setMinWidth(int newMinWidth);

private:
    QStringList splitArgs(QString args);
private:
    QLabel* mInfoLabel;
    QLabel* mTotalLabel;
    QToolButton* mUpButton;
    QToolButton* mDownButton;
    int mParamPos;
    int mInfoIndex;
    int mParamIndex;
    QString mFunctioFullName;

    QList<PFunctionInfo> mInfos;
    int mMinWidth;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
};

#endif // FUNCTIONTOOLTIPWIDGET_H
