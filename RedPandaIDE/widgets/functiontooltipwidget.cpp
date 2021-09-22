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

QStringList FunctionTooltipWidget::splitArgs(const QString &argStr)
{
    // Split up argument string by ,
    int i;
    if (argStr.startsWith('(')) {
        i = 1; // assume it starts with ( and ends with )
    } else {
        i = 0;
    }
    int paramStart = i;

    QStringList result;
    while (i < argStr.length()) {
        if ((argStr[i] == ',') ||
                ((i == argStr.length()-1) && (argStr[i] == ')'))) {
            // We've found "int* a" for example
            QString s = argStr.mid(paramStart,i-paramStart);

            //remove default value
            int assignPos = s.indexOf('=');
            if (assignPos >= 0) {
                s.truncate(assignPos);
                s = s.trimmed();
            }
            // we don't support function pointer parameters now, till we can tokenize function parameters
//        {
//        // Can be a function pointer. If so, scan after last )
//        BracePos := LastPos(')', S);
//        if (BracePos > 0) then // it's a function pointer... begin
//          SpacePos := LastPos(' ', Copy(S, BracePos, MaxInt)) // start search at brace
//        end else begin
//        }
            int spacePos = s.lastIndexOf(' '); // Cut up at last space
            if (spacePos >= 0) {
                args = "";
                int bracketPos = s.indexOf('[');
                if (bracketPos >= 0) {
                    args = s.mid(bracketPos);
                    s.truncate(bracketPos);
                }
                addStatement(
                            functionStatement,
                            mCurrentFile,
                            "", // do not override hint
                            s.mid(0,spacePos), // 'int*'
                            s.mid(spacePos+1), // a
                            args,
                            "",
                            functionStatement->definitionLine,
                            StatementKind::skParameter,
                            StatementScope::ssLocal,
                            StatementClassScope::scsNone,
                            true,
                            false);
            }
            paramStart = i + 1; // step over ,
        }
        i++;
    }
    return result;
}
