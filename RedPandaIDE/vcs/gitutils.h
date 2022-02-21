#ifndef GITUTILS_H
#define GITUTILS_H

#include <QDateTime>
#include <QString>
#include <memory>


enum class GitResetStrategy {
    Soft,
    Hard,
    Merge,
    Mixed,
    Keep
};

enum class GitMergeStrategy {
    Resolve,
    Recursive,
    Ours,
    Subtree
};

enum class GitMergeStrategyOption {
    Ours,
    Theirs,
    Patience,
    Ignore_Space_Change,
    Ignore_All_Space,
    Ignore_Space_At_Eol,
    Renormalize,
    No_Renormalize,
    Find_Names,
    Rename_Threshold,
    Subtree
};

struct GitCommitInfo {
    QString commitHash;
    QString author;
    QDateTime authorDate;
    QString title;
    QString fullCommitMessage;
};

using PGitCommitInfo = std::shared_ptr<GitCommitInfo>;

#endif // GITUTILS_H
