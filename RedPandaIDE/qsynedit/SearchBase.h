#ifndef SYNSEARCHBASE_H
#define SYNSEARCHBASE_H

#include <QObject>
#include <memory>

enum SynSearchOption {
    ssoMatchCase, ssoWholeWord, ssoBackwards,
    ssoEntireScope, ssoSelectedOnly, ssoReplace, ssoReplaceAll, ssoPrompt,
    ssoRegExp};

Q_DECLARE_FLAGS(SynSearchOptions, SynSearchOption)
Q_DECLARE_OPERATORS_FOR_FLAGS(SynSearchOptions)

class SynSearchBase : public QObject
{
    Q_OBJECT
public:
    explicit SynSearchBase(QObject *parent = nullptr);
    QString pattern();
    virtual void setPattern(const QString& value);
    virtual int length(int aIndex) = 0;
    virtual int result(int aIndex) = 0;
    virtual int resultCount() = 0;
    virtual int findAll(const QString& newText) = 0;
    virtual QString replace(const QString& aOccurrence, const QString& aReplacement) = 0;
    SynSearchOptions options() const;
    virtual void setOptions(const SynSearchOptions &options);

private:
    QString mPattern;
    SynSearchOptions mOptions;
};

using PSynSearchBase = std::shared_ptr<SynSearchBase>;

#endif // SYNSEARCHBASE_H
