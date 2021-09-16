#ifndef PROJECTTEMPLATE_H
#define PROJECTTEMPLATE_H

#include <QObject>
#include "utils.h"
#include "projectoptions.h"

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
    int addUnit();
    void readTemplateFile(const QString& fileName);
    bool save();
    const QString &category() const;
    void setCategory(const QString &newCategory);

    const QString &description() const;
    void setDescription(const QString &newDescription);

    const QString &fileName() const;
    void setFileName(const QString &newFileName);

    const QString &icon() const;
    void setIcon(const QString &newIcon);

    const QString &name() const;
    void setName(const QString &newName);

    const ProjectOptions &options() const;
    void setOptions(const ProjectOptions &newOptions);

private:
    QString mFileName;
    ProjectOptions mOptions;
    QString mDescription;
    QString mCategory;
    QString mName;
    QString mIcon; // icon in project form
    PSimpleIni mIni;
    int mVersion;
signals:

};
using PProjectTemplate = std::shared_ptr<ProjectTemplate>;

#endif // PROJECTTEMPLATE_H
