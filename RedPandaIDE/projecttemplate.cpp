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
#include "projecttemplate.h"
#include <QFile>
#include <QMessageBox>
#include "mainwindow.h"
#include "settings.h"

ProjectTemplate::ProjectTemplate(QObject *parent) : QObject(parent)
{

}

int ProjectTemplate::unitCount()
{
    if (!mIni || mVersion<=0)
        return 0;
    return mIni->GetLongValue("Project","UnitCount",0);
}

PTemplateUnit ProjectTemplate::unit(int index)
{
    if (!mIni || mVersion<=0)
        return PTemplateUnit();
    QString section = QString("Unit%1").arg(index);
    PTemplateUnit unit = std::make_shared<TemplateUnit>();
    unit->Source = fromByteArray(mIni->GetValue(toByteArray(section), "Source", ""));
    unit->Target = fromByteArray(mIni->GetValue(toByteArray(section), "Target", ""));
    unit->CText = fromByteArray(mIni->GetValue(toByteArray(section), "C", ""));
    unit->CppText = fromByteArray(mIni->GetValue(toByteArray(section), "Cpp", ""));
    if (unit->CppText.isEmpty())
        unit->CppText = unit->CText;

    unit->CName = fromByteArray(mIni->GetValue(toByteArray(section), "CName", ""));
    unit->CppName = fromByteArray(mIni->GetValue(toByteArray(section), "CppName", ""));
    if (unit->CppName.isEmpty())
        unit->CppName = unit->CName;
    return unit;
}

void ProjectTemplate::setUnit(int index, PTemplateUnit newUnit)
{
    if (!mIni || mVersion<=0)
        return;
    QByteArray section = toByteArray(QString("Unit%1").arg(index));
    mIni->SetValue(section,"C", toByteArray(newUnit->CText));
    mIni->SetValue(section,"Cpp", toByteArray(newUnit->CppText));
    mIni->SetValue(section,"CName", toByteArray(newUnit->CName));
    mIni->SetValue(section,"CppName", toByteArray(newUnit->CppName));
    mIni->SetValue(section,"Source", toByteArray(newUnit->Source));
    mIni->SetValue(section,"Target", toByteArray(newUnit->Target));
}

int ProjectTemplate::addUnit()
{
    if (!mIni || mVersion<=0)
        return -1;
    int count = unitCount() +1;
    QByteArray section = toByteArray(QString("Unit%1").arg(count-1));
    mIni->SetValue(section, "C", "");
    mIni->SetValue(section, "Cpp", "");
    mIni->SetLongValue("Project", "UnitCount", count);
    return count;
}

void ProjectTemplate::readTemplateFile(const QString &fileName)
{
    if (mIni)
        mIni=nullptr;
    if (QFile(fileName).exists()) {
        mFileName = fileName;
        mIni = std::make_shared<SimpleIni>();
        if (mIni->LoadFile(mFileName.toLocal8Bit()) != SI_OK) {
            QMessageBox::critical(pMainWindow,
                                  tr("Read failed."),
                                  tr("Can't read template file '%1'.").arg(fileName),
                                  QMessageBox::Ok);
            return;
        }
    } else {
        QMessageBox::critical(pMainWindow,
                              tr("Template not exist"),
                              tr("Template file '%1' doesn't exist.").arg(fileName),
                              QMessageBox::Ok);
        return;
    }

    mVersion = mIni->GetLongValue("Template", "Ver", 0);
    if (mVersion<=0) {
        QMessageBox::critical(pMainWindow,
                              tr("Old version template"),
                              tr("Template file '%1' has version '%2', which is unsupported.")
                              .arg(fileName)
                              .arg(mVersion),
                              QMessageBox::Ok);
        return;
    }

    QString lang = pSettings->environment().language();
    // template info
    mIcon = fromByteArray(mIni->GetValue("Template", "Icon", ""));
    if (!lang.isEmpty()) {
        mCategory = fromByteArray(mIni->GetValue("Template", QString("Category[%1]").arg(lang).toUtf8(), ""));
        mName = fromByteArray(mIni->GetValue("Template", QString("Name[%1]").arg(lang).toUtf8(), ""));
        mDescription = fromByteArray(mIni->GetValue("Template", QString("Description[%1]").arg(lang).toUtf8(), ""));
    }
    if (mCategory.isEmpty())
        mCategory = fromByteArray(mIni->GetValue("Template", "Category", ""));
    if (mName.isEmpty())
        mName = fromByteArray(mIni->GetValue("Template", "Name", ""));
    if (mDescription.isEmpty())
        mDescription = fromByteArray(mIni->GetValue("Template", "Description", ""));

    mOptions.icon = mIni->GetValue("Project", "Icon", "");
    mOptions.type = static_cast<ProjectType>(mIni->GetLongValue("Project", "Type", 0)); // default = gui
    mOptions.objFiles  = fromByteArray(mIni->GetValue("Project", "ObjFiles", "")).split(";",QString::SkipEmptyParts);
    mOptions.includes = fromByteArray(mIni->GetValue("Project", "Includes", "")).split(";",QString::SkipEmptyParts);
    mOptions.libs = fromByteArray(mIni->GetValue("Project", "Libs", "")).split(";",QString::SkipEmptyParts);
    mOptions.resourceIncludes = fromByteArray(mIni->GetValue("Project", "ResourceIncludes", "")).split(";",QString::SkipEmptyParts);
    mOptions.compilerCmd = fromByteArray(mIni->GetValue("Project", "Compiler", ""));
    mOptions.cppCompilerCmd = fromByteArray(mIni->GetValue("Project", "CppCompiler", ""));
    mOptions.linkerCmd = fromByteArray(mIni->GetValue("Project", "Linker",""));
    mOptions.isCpp = mIni->GetBoolValue("Project", "IsCpp", false);
    mOptions.includeVersionInfo = mIni->GetBoolValue("Project", "IncludeVersionInfo", false);
    mOptions.supportXPThemes = mIni->GetBoolValue("Project", "SupportXPThemes", false);
    mOptions.exeOutput = fromByteArray(mIni->GetValue("Project", "ExeOutput", ""));
    mOptions.objectOutput = fromByteArray(mIni->GetValue("Project", "ObjectOutput", ""));
    mOptions.logOutput = fromByteArray(mIni->GetValue("Project", "LogOutput", ""));
    mOptions.staticLink  = mIni->GetBoolValue("Project", "StaticLink",true);
    mOptions.addCharset  = mIni->GetBoolValue("Project", "AddCharset",true);
    bool useUTF8 = mIni->GetBoolValue("Project", "UseUTF8", false);
    if (useUTF8) {
        mOptions.encoding = fromByteArray(mIni->GetValue("Project","Encoding", ENCODING_UTF8));
    } else {
        mOptions.encoding = fromByteArray(mIni->GetValue("Project","Encoding", ENCODING_AUTO_DETECT));
    }
    mOptions.modelType = (ProjectModelType)mIni->GetLongValue("Project", "ModelType", (int)ProjectModelType::FileSystem);

}

bool ProjectTemplate::save()
{
  if (mIni) {
      return mIni->SaveFile(toByteArray(mFileName)) == SI_OK ;
  }
  return false;
}

const QString &ProjectTemplate::category() const
{
    return mCategory;
}

void ProjectTemplate::setCategory(const QString &newCategory)
{
    mCategory = newCategory;
}

const QString &ProjectTemplate::description() const
{
    return mDescription;
}

void ProjectTemplate::setDescription(const QString &newDescription)
{
    mDescription = newDescription;
}

const QString &ProjectTemplate::fileName() const
{
    return mFileName;
}

void ProjectTemplate::setFileName(const QString &newFileName)
{
    mFileName = newFileName;
}

const QString &ProjectTemplate::icon() const
{
    return mIcon;
}

void ProjectTemplate::setIcon(const QString &newIcon)
{
    mIcon = newIcon;
}

const QString &ProjectTemplate::name() const
{
    return mName;
}

void ProjectTemplate::setName(const QString &newName)
{
    mName = newName;
}

const ProjectOptions &ProjectTemplate::options() const
{
    return mOptions;
}

void ProjectTemplate::setOptions(const ProjectOptions &newOptions)
{
    mOptions = newOptions;
}

int ProjectTemplate::version() const
{
    return mVersion;
}

