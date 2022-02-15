#ifndef GITUTILS_H
#define GITUTILS_H

enum class GitResetStrategy {
    Soft,
    Hard,
    Merge,
    Mixed,
    Keep
};

#endif // GITUTILS_H
