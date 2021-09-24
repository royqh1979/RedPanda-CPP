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
    void nextTip();
    void previousTip();
    void updateTip();

    int paramIndex() const;
    void setParamIndex(int newParamIndex);

    const QString &functionFullName() const;
    void setFunctioFullName(const QString &newFunctioFullName);

signals:
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

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
};

#endif // FUNCTIONTOOLTIPWIDGET_H
