#ifndef FUNCTIONTOOLTIPWIDGET_H
#define FUNCTIONTOOLTIPWIDGET_H
#include <QLabel>
#include <QToolButton>
#include <QWidget>

struct FunctionInfo {
    QString name;
    QString fullname;
    QStringList params;
    QStringList nonameParams;
    QString returnType;
};

using PFunctionInfo = std::shared_ptr<FunctionInfo>;

class FunctionTooltipWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FunctionTooltipWidget(QWidget *parent = nullptr);
    void addTip(const QString& name, const QString& fullName,
                const QString& returnType, const QString& args,
                const QString& noNameArgs);
    void clearTips();
    int paramPos() const;
    void setParamPos(int newParamPos);

    int index() const;
    void setIndex(int newIndex);
signals:
private:
    QStringList splitArgs(QString args);
private:
    QLabel* mInfoLabel;
    QLabel* mTotalLabel;
    QToolButton* mUpButton;
    QToolButton* mDownButton;
    int mParamPos;
    int mIndex;
    QList<PFunctionInfo> mInfos;
};

#endif // FUNCTIONTOOLTIPWIDGET_H
