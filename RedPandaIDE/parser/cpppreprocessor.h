#ifndef CPPPREPROCESSOR_H
#define CPPPREPROCESSOR_H

#include <QObject>

class CppPreprocessor : public QObject
{
    Q_OBJECT
public:
    explicit CppPreprocessor(QObject *parent = nullptr);

signals:

};

#endif // CPPPREPROCESSOR_H
