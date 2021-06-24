#ifndef COMMON_H
#define COMMON_H
#include <QString>
#include <memory>
#include <QMetaType>
enum class CompileIssueType {
    Other,
    Warning,
    Info,
    Note,
    Error,
};

struct CompileIssue {
    QString filename;
    int line;
    int column;
    int endColumn;
    QString description;
    CompileIssueType type;
};

typedef std::shared_ptr<CompileIssue> PCompileIssue;

Q_DECLARE_METATYPE(PCompileIssue);

#endif // COMMON_H
