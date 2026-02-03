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
        virtual int calcIndentSpaces(int line, const QString& lineText, bool addIndent,
                             const QSynEdit *editor)=0;
    };
    using PFormatter = std::shared_ptr<Formatter>;
}

#endif // FORMATTER_H
