#ifndef QSYNEDIT_FORMATTER_H
#define QSYNEDIT_FORMATTER_H
#include "../types.h"
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariant>
namespace QSynedit {
    class QSynEdit;
    class Formatter
    {
    public:
        Formatter();
        virtual ProgrammingLanguage supportLanguage()=0;
        const QStringList &optionNames() const;
        void setOption(const QString& name, QVariant val);
        QVariant getOption(const QString& name, const QVariant& defaultValue=QVariant()) const;
        bool getBoolOption(const QString& name,bool defaultValue) const;
        QString getStringOption(const QString& name,const QString& defaultValue) const;
        int getIntOption(const QString& name,int defaultValue) const;
        virtual int calcIndentSpaces(int line, const QString& lineText, bool addIndent,
                             const QSynEdit *editor)=0;
        void initOptions();
    protected:
        virtual void doInitOptions() = 0;
    protected:
        QMap<QString,QVariant> mOptions;
        QStringList mOptionNames;
    };
    using PFormatter = std::shared_ptr<Formatter>;
}

#endif // FORMATTER_H
