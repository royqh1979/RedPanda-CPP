#ifndef PROJECTTEMPLATE_H
#define PROJECTTEMPLATE_H

#include <QObject>
#include "project.h"

struct TemplateUnit {
  QString CName;
  QString CppName;
  QString CText;
  QString CppText;
};

using PTemplateUnit = std::shared_ptr<TemplateUnit>;

class ProjectTemplate : public QObject
{
    Q_OBJECT
public:
    explicit ProjectTemplate(QObject *parent = nullptr);
    int unitCount();
    PTemplateUnit unit(int index);
    void setUnit(int index, PTemplateUnit newUnit);

private:
    QString mFileName;
    ProjectOptions mOptions;
    QString mDesc;
    QString mCategory;
    QString mName;
    QString mIcon; // icon in project form
    int mVersion;
signals:

};

#endif // PROJECTTEMPLATE_H
