/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
  QString Source;
  QString Target;
  bool overwrite;
};

using PTemplateUnit = std::shared_ptr<TemplateUnit>;

class ProjectTemplate : public QObject
{
    Q_OBJECT
public:
    explicit ProjectTemplate(QObject *parent = nullptr);
    int unitCount();
    PTemplateUnit unit(int index);
    void readTemplateFile(const QString& fileName);
    bool save();
    const QString &category() const;
    void setCategory(const QString &newCategory);

    const QString &description() const;
    void setDescription(const QString &newDescription);

    const QString &fileName() const;
    void setFileName(const QString &newFileName);

    const QString folder() const;

    const QString &icon() const;
    void setIcon(const QString &newIcon);

    const QString &name() const;
    void setName(const QString &newName);

    const ProjectOptions &options() const;
    void setOptions(const ProjectOptions &newOptions);

    int version() const;

    void setVersion(int newVersion);

    QString iconInfo() const;
    void setIconInfo(const QString &newIconInfo);

private:
    QString mFileName;
    ProjectOptions mOptions;
    QString mDescription;
    QString mCategory;
    QString mName;
    QString mIcon; // icon in project form
    QString mIconInfo;
    PSimpleIni mIni;
    int mVersion;
};
using PProjectTemplate = std::shared_ptr<ProjectTemplate>;

#endif // PROJECTTEMPLATE_H
